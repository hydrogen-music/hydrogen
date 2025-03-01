package main

import (
    "context"
    "log"
    "os"
    "os/exec"
    "path"
    "path/filepath"
    "strconv"
    "time"

    "github.com/hypebeast/go-osc/osc"
)

const oscHydrogenPort = 9099
const oscTestBinaryPort = 8099
const oscTestBinaryListenerPort = 8100

// hydrogenStartupTime gives an upper limit for the time Hydrogen requires to
// start up in milliseconds.
const hydrogenStartupTime = 7000
// hydrogenTearDownTime gives an upper limit for the time required for
// killHydrogen() to send a quit OSC signal, for Hydrogen to receive it and
// finish its tear down.
const hydrogenTearDownTime = 7000

// oscCommandTime defines the time in milliseconds the tests do wait after
// sending an OSC command before going on.
const oscCommandTime = 800

var hydrogenLogFile = "./hydrogen.log"
var hydrogenConfFile = "./hydrogen.conf"
var testBinaryLogFileBase = "./test"

// hydrogenTestSongPath points to the song opened by the Hydrogen binary used as
// counterpart to our dedicated test binary.
var hydrogenTestSongPath, _ = filepath.Abs("./jackTimebaseTest.h2song")
// var hydrogenTestSongListenerPath, _ =
//     filepath.Abs("./jackTimebaseTest_listener.h2song")
var hydrogenTestSongListenerPath, _ =
    filepath.Abs("./jackTimebaseTest.h2song")

// hydrogenFailedChan is used by startHydrogen() to indicate that Hydrogen exited.
var hydrogenFailedChan chan bool
// testBinaryFailedChan is used by startTestBinary() to indicate it exited.
var testBinaryFailedChan chan bool
// nextTestChan is used by startTestBinary() to indicate it exited.
var nextTestChan chan bool
var finishTestChan chan bool

var hydrogenPath = path.Join(
    "..", "..", "build", "src", "cli", "h2cli")
var testBinaryPath = path.Join(
    "..", "..", "build", "tests", "jackTimebase", "h2JackTimebase", "h2JackTimebase")


var hydrogenClient *osc.Client
var testBinaryClient *osc.Client
var testBinaryListenerClient *osc.Client

var testNumber int

var returnCode int

// Integration test checking for errors/crashes when closing Hydrogen while
// using the JACK driver (on Linux).
func main() {
    var err error

    _, err = exec.LookPath(hydrogenPath)
    if err != nil {
        log.Fatalf("[main] [%v] executable could not be found: %v", hydrogenPath,
            err.Error())
    }
    _, err = exec.LookPath(testBinaryPath)
    if err != nil {
        log.Fatalf("[main] [%v] executable could not be found: %v", testBinaryPath,
            err.Error())
    }

    hydrogenFailedChan = make(chan bool, 1)
    testBinaryFailedChan = make(chan bool, 1)
    nextTestChan = make(chan bool, 1)
    finishTestChan = make(chan bool, 1)

    // Trigger first test
    nextTestChan <- true
    testNumber = 0
    returnCode = 0

    testContext, testContextCancel := context.WithCancel(context.Background())

    hydrogenClient = osc.NewClient("localhost", oscHydrogenPort)
    testBinaryClient = osc.NewClient("localhost", oscTestBinaryPort)
    testBinaryListenerClient = osc.NewClient("localhost", oscTestBinaryListenerPort)

    go startHydrogen(testContext)

    // We have to wait until Hydrogen is up and running. Else setting its
    // timebase state might fail since it has no song set yet.
    time.Sleep(hydrogenStartupTime * time.Millisecond)

    mainLoop(testContext)

    log.Println("[main] Initiating teardown...")

    // Teardown
    sendMsg(hydrogenClient, osc.NewMessage("/Hydrogen/QUIT"))
    sendMsg(testBinaryClient, osc.NewMessage("/Hydrogen/QUIT"))
    sendMsg(testBinaryListenerClient, osc.NewMessage("/Hydrogen/QUIT"))

    // Give both applications time to shut down gracefully
    time.Sleep(hydrogenTearDownTime * time.Millisecond)

    testContextCancel()

    time.Sleep(100 * time.Millisecond)

    os.Exit( returnCode )
 }

func mainLoop(ctx context.Context) {
    for {
        select {
        case <-hydrogenFailedChan:
            // Hydrogen exited
            log.Println("[mainLoop] hydrogenFailedChan")
            returnCode = 1
            return
        case <-testBinaryFailedChan:
            // Test binary exits
            log.Println("[mainLoop] testBinaryFailedChan")
            returnCode = 1
            return
        case <-nextTestChan:
            // Integration test succeeded
            nextTest(ctx)

        case <-finishTestChan:
            // Done
            return

        default:
            time.Sleep(100 * time.Millisecond)
            continue
        }
    }
}

func sendMsg(c *osc.Client, m *osc.Message) {

    log.Printf("[sendMsg] sending message [%v] to [:%v]",
        m.String(), c.Port())

    c.Send(m)

    // Wait for Hydrogen to receive and handle the message.
    time.Sleep(oscCommandTime * time.Millisecond)
}

func startHydrogen(ctx context.Context) {
    cmd := exec.CommandContext(ctx, hydrogenPath, "--driver", "jack",
        "-s", hydrogenTestSongPath, "-O", strconv.FormatInt(oscHydrogenPort, 10),
        "--config", hydrogenConfFile, "-L", hydrogenLogFile, "-T", "-V", "Debug")
    output, err := cmd.CombinedOutput()
    if err == nil {
        log.Printf("[startHydrogen] [%v] exited", cmd.String())
    } else {
        log.Printf("[startHydrogen] ERROR: [%v] exited with error: %v",
            cmd.String(), err)
        log.Printf("[startHydrogen] stdout/stderr: %v", string(output))
        hydrogenFailedChan <- true
    }
}

func startTestBinary(ctx context.Context, logFileSuffix string,
    timebaseState int64, oscPort int64, testFile string) {

    log.Println("[startTestBinary] Starting test binary...")

    cmd := exec.CommandContext(ctx, testBinaryPath,
        "-L", testBinaryLogFileBase + "-" + logFileSuffix + ".log",
        "--timebase-state", strconv.FormatInt(timebaseState, 10),
        "--config", hydrogenConfFile, "-s", testFile,
        "-O", strconv.FormatInt(oscPort, 10), "-V", "Debug")
    output, err := cmd.CombinedOutput()
    if err == nil {
        log.Println("[startTestBinary] SUCCESS!")
        log.Println("")
        nextTestChan <- true
    } else {
        log.Printf("[startTestBinary] ERROR: [%v] exited with error: %v",
            cmd.String(), err)
        log.Printf("[startTestBinary] stdout/stderr: %v", string(output))
        log.Println("")
        testBinaryFailedChan <- true
    }
}

func nextTest(ctx context.Context) {
    switch(testNumber) {
    case 0:
        log.Println("[nextTest] Two regular JACK clients")

        // Update timebase state of reference application. Note: the state of
        // the JACK driver is only update while transport is rolling
        sendMsg(hydrogenClient, osc.NewMessage("/Hydrogen/PLAY"))
        sendMsg(hydrogenClient,
            osc.NewMessage("/Hydrogen/JACK_TIMEBASE_MASTER_ACTIVATION", float64(0)))
        sendMsg(hydrogenClient, osc.NewMessage("/Hydrogen/STOP"))

        go startTestBinary(ctx, "non-timebase", -1, oscTestBinaryPort,
            hydrogenTestSongPath)
        // Wait for the test binary to be ready.
        time.Sleep(hydrogenStartupTime * time.Millisecond)

        sendMsg(testBinaryClient, osc.NewMessage("/h2JackTimebase/TransportTests"))

    case 1:
        log.Println("[nextTest] Test binary as Timebase controller")
        // The test binary is registered as JACK Timebase controller and the
        // reference application as listener.

        // Update timebase state of reference application. Note: the state of
        // the JACK driver is only update while transport is rolling
        sendMsg(hydrogenClient, osc.NewMessage("/Hydrogen/PLAY"))
        sendMsg(hydrogenClient,
            osc.NewMessage("/Hydrogen/JACK_TIMEBASE_MASTER_ACTIVATION", float64(0)))
        sendMsg(hydrogenClient, osc.NewMessage("/Hydrogen/STOP"))

        go startTestBinary(ctx, "timebase-controller", 1, oscTestBinaryPort,
            hydrogenTestSongPath)
        // Wait for the test binary to be ready.
        time.Sleep(hydrogenStartupTime * time.Millisecond)

        sendMsg(testBinaryClient, osc.NewMessage("/h2JackTimebase/TransportTests"))

    case 2:
        log.Println("[nextTest] Test binary as Timebase listener")
        // The reference application is registered as JACK Timebase controller
        // and the test binary as listener.
        //
        // Note that this test does not check transport repositing using BBT
        // information. All relocations are triggered by the listener (test
        // binary) and won't have a valid BBT position bit.

        // Update timebase state of reference application. Note: the state of
        // the JACK driver is only update while transport is rolling.
        sendMsg(hydrogenClient, osc.NewMessage("/Hydrogen/PLAY"))
        sendMsg(hydrogenClient,
            osc.NewMessage("/Hydrogen/JACK_TIMEBASE_MASTER_ACTIVATION", float64(1)))
        sendMsg(hydrogenClient, osc.NewMessage("/Hydrogen/STOP"))

       go startTestBinary(ctx, "timebase-listener", 0, oscTestBinaryPort,
            hydrogenTestSongListenerPath)
        // Wait for the test binary to be ready.
        time.Sleep(hydrogenStartupTime * time.Millisecond)

        sendMsg(testBinaryClient, osc.NewMessage("/h2JackTimebase/TransportTests"))

    case 3:
        log.Println("[nextTest] BBT relocation test")

        // Start a dedicated test binary instead of the main application. We
        // want to test whether relocations happening within the controller are
        // properly handled when received by the listener (relocate using the
        // actual BBT information). For this we need two applications with
        // patched JACK process callback.
        sendMsg(hydrogenClient, osc.NewMessage("/Hydrogen/QUIT"))

        go startTestBinary(ctx, "bbt-relocation-listener", 0,
            oscTestBinaryListenerPort, hydrogenTestSongListenerPath)
        go startTestBinary(ctx, "bbt-relocation-controller", 1, oscTestBinaryPort,
            hydrogenTestSongPath )
        // Wait for the test binary to be ready.
        time.Sleep(hydrogenStartupTime * time.Millisecond)

        sendMsg(testBinaryListenerClient,
            osc.NewMessage("/h2JackTimebase/StartTestJackDriver"))
        sendMsg(testBinaryClient, osc.NewMessage("/h2JackTimebase/TransportTests"))

    default:
        log.Println("[nextTest] No test left. Exiting...")
        finishTestChan <- true
    }

    testNumber += 1
}

package main

import (
    "context"
    "log"
    "os/exec"
    "path"
    "strconv"
    "time"

    "github.com/hypebeast/go-osc/osc"
)

const oscHydrogenPort = 9099
const oscTestBinaryPort = 8099

// hydrogenStartupTime gives an upper limit for the time Hydrogen requires to
// start up in milliseconds.
const hydrogenStartupTime = 3700
// hydrogenTearDownTime gives an upper limit for the time required for
// killHydrogen() to send a quit OSC signal, for Hydrogen to receive it and
// finish its tear down.
const hydrogenTearDownTime = 3000

var hydrogenLogFile = "./hydrogen.log"
var testBinaryLogFile = "./test.log"

// hydrogenChan is used by startHydrogen() to indicate that Hydrogen exited.
var hydrogenChan chan bool
// testBinaryChan is used by startTestBinary() to indicate it exited.
var testBinaryChan chan bool

var hydrogenPath = path.Join(
    "..", "..", "build", "src", "gui", "hydrogen")
var testBinaryPath = path.Join(
    "..", "..", "build", "tests", "jackTimebase", "h2JackTimebase", "h2JackTimebase")

// Integration test checking for errors/crashes when closing Hydrogen while
// using the JACK driver (on Linux).
func main() {
    var err error

    _, err = exec.LookPath(hydrogenPath)
    if err != nil {
        log.Fatalf("[%v] executable could not be found: %v", hydrogenPath,
            err.Error())
    }
    _, err = exec.LookPath(testBinaryPath)
    if err != nil {
        log.Fatalf("[%v] executable could not be found: %v", testBinaryPath,
            err.Error())
    }

    hydrogenChan = make(chan bool, 1)
    testBinaryChan = make(chan bool, 1)

    testContext, testContextCancel := context.WithCancel(context.Background())

    hydrogenClient := osc.NewClient("localhost", oscHydrogenPort)
    testBinaryClient := osc.NewClient("localhost", oscTestBinaryPort)

    go startHydrogen(testContext)
    go startTestBinary(testContext)

    time.Sleep(hydrogenStartupTime * time.Millisecond)

    mainLoop()

    log.Println("Initiating teardown...")


    // Teardown
    msg := osc.NewMessage("/Hydrogen/QUIT")
    hydrogenClient.Send(msg)
    testBinaryClient.Send(msg)

    // Give both applications time to shut down gracefully
    time.Sleep(hydrogenTearDownTime * time.Millisecond)

    testContextCancel()

    time.Sleep(100 * time.Millisecond)
 }

func mainLoop() {
    for {
        select {
        case <-hydrogenChan:
            // Hydrogen exited
            return
        case <-testBinaryChan:
            // Test binary exits
            return

        default:
            time.Sleep(100 * time.Millisecond)
            continue
        }
    }
}

func startHydrogen(ctx context.Context) {
    cmd := exec.CommandContext(ctx, hydrogenPath, "--driver", "jack", "--nosplash",
        "-O", strconv.FormatInt(oscHydrogenPort, 10), "-L", hydrogenLogFile, "-T",
        "-V", "Debug")
    err := cmd.Run()
    if err == nil {
        log.Printf("[startHydrogen] [%v] exited", cmd.String())
    } else {
        log.Printf("[startHydrogen] ERROR: [%v] exited with error: %v",
            cmd.String(), err)
    }

    hydrogenChan <- true
}

func startTestBinary(ctx context.Context) {
    cmd := exec.CommandContext(ctx, testBinaryPath, "-L", testBinaryLogFile,
        "-O", strconv.FormatInt(oscTestBinaryPort, 10), "-V", "Debug")
    err := cmd.Run()
    if err == nil {
        log.Printf("[startTestBinary] [%v] exited", cmd.String())
    } else {
        log.Printf("[startTestBinary] ERROR: [%v] exited with error: %v",
            cmd.String(), err)
    }

    testBinaryChan <- true
}

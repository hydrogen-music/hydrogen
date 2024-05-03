package main

import (
    "context"
    "fmt"
    "log"
    "os/exec"
    "path"
    "strconv"
    "time"

    "github.com/hypebeast/go-osc/osc"
)

// How many times the test should be repeated.
const numberOfTestRuns = 100
const oscHydrogenPort = 7777

// hydrogenStartupTime gives an upper limit for the time Hydrogen requires to
// start up in milliseconds.
const hydrogenStartupTime = 3700
// hydrogenTearDownTime gives an upper limit for the time required for
// killHydrogen() to send a quit OSC signal, for Hydrogen to receive it and
// finish its tear down.
const hydrogenTearDownTime = 5000

var hydrogenPath = path.Join(
    "..", "..", "build", "src", "cli", "h2cli")

// hydrogenStartupChan is used by startHydrogen() to indicate that Hydrogen was
// started.
var hydrogenStartupChan chan bool

// Integration test checking for errors/crashes when closing Hydrogen while
// using the JACK driver (on Linux).
func main() {
    var err error

    _, err = exec.LookPath(hydrogenPath)
    if err != nil {
        log.Fatalf("[h2cli] executable could not be found: %v", err.Error())
    }

    oscClient := osc.NewClient("localhost", oscHydrogenPort)

    hydrogenStartupChan = make(chan bool, 1)

    for ii := 0; ii < numberOfTestRuns; ii++ {

        go killHydrogen(oscClient)

        err = startHydrogen()

        if err != nil {
            log.Fatalf("Hydrogen exited with non-zero code: %v",
                err.Error())
        }
        log.Printf("Instance [%v] terminated without error\n", ii)
    }
}

// Start up Hydrogen using the JACK driver and with a configuration that both
// enables OSC and uses a specific port unlikely used by another Hydrogen
// instance (in case another one is already running).
func startHydrogen() error {
    // Wait a couple of second till the program is killed. This is required
    // since a crash of Hydrogen (what we want expect in the test) would result
    // in a zombie process.
    ctx, _ := context.WithTimeout(context.Background(),
        (hydrogenTearDownTime + hydrogenStartupTime) * time.Millisecond)
    cmd := exec.CommandContext(ctx, hydrogenPath, "--driver", "jack",
        "-O", strconv.FormatInt(oscHydrogenPort, 10), "-T", "-L", "./h2cli.log")

    hydrogenStartupChan <- true

    output, err := cmd.Output()
    if err != nil {
        return fmt.Errorf("[startHydrogen] Exited with error [%v]:\n%v\n\n",
            err, string(output))
    }

    return nil
}

func killHydrogen(client *osc.Client) {
    for {
        select {
        case <-hydrogenStartupChan:
            // Give Hydrogen some time to start up properly
            time.Sleep(hydrogenStartupTime * time.Millisecond)

            // Hydrogen is ready. Let's shut it down.
            msg := osc.NewMessage("/Hydrogen/QUIT")
            err := client.Send(msg)
            if err != nil {
                log.Fatalf("[killHydrogen] Unable to send OSC message: %v",
                    err.Error())
            }

            return

        default:
            time.Sleep(100 * time.Millisecond)
            continue
        }
    }
}

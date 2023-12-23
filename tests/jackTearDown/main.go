package main

import (
    "fmt"
    "log"
    "os/exec"
    "time"

    "github.com/hypebeast/go-osc/osc"
)

// How many times the test should be repeated.
const numberOfTestRuns = 100
const oscHydrogenPort = 9000

// hydrogenStartupTime gives an upper limit for the time Hydrogen requires to
// start up in milliseconds.
const hydrogenStartupTime = 3000

// hydrogenStartupChan is used by startHydrogen() to indicate that Hydrogen was
// started.
var hydrogenStartupChan chan bool

// Integration test checking for errors/crashes when closing Hydrogen while
// using the JACK driver (on Linux).
func main() {
    var err error

    _, err = exec.LookPath("hydrogen")
    if err != nil {
        log.Fatalf("[hydrogen] executable could not be found: %v", err.Error())
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
    hydrogenStartupChan <- true

    cmd := exec.Command("hydrogen", "--driver", "jack", "--nosplash")
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

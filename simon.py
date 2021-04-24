import machine
import utime
import random

class SimonIO:
    def __init__(self, name, ledPin, buttonPin, buzzerFreq):
        self.name = name
        self.led = machine.Pin(ledPin, machine.Pin.OUT)
        self.button = machine.Pin(buttonPin, machine.Pin.IN, machine.Pin.PULL_DOWN)
        self.buzzerFreq = buzzerFreq

    def buzz(self, enable):
        if enable:
            buzzer.duty_u16(1000)
            buzzer.freq(self.buzzerFreq)
        else:
            buzzer.duty_u16(0)

    def show(self, timeout):
        self.led.on()
        self.buzz(True)
        utime.sleep(timeout)
        self.led.off()
        self.buzz(False)

pins = [
    SimonIO("Green", 16, 17, 262), # green, C4
    SimonIO("Red", 18, 19, 294), # red, D4
    SimonIO("Yellow", 20, 21, 330), # yellow, E4
    SimonIO("Blue", 26, 27, 349), # blue, F4
]

buzzer = machine.PWM(machine.Pin(28))

# There's no escape!
while True:
    # Reset game to initial state.
    print("Resetting...")
    for pin in pins:
        pin.led.off()
    buzzer.duty_u16(0)
    currentGame = []

    # Run a sort of attract mode.
    print("Running attract...")
    for _ in range(2):
        for pin in pins:
            pin.led.on()
            pin.buzz(True)
            utime.sleep(.1)

        for pin in reversed(pins):
            pin.led.off()
            pin.buzz(False)
            utime.sleep(.1)

    for _ in range(4):
        for pin in pins:
            pin.led.toggle()
        utime.sleep(.1)

    lost = False

    while not lost:
        # Add a new entry.
        newPin = random.choice(pins)
        currentGame.append(newPin)
        print("Adding new entry %s..." % newPin.name)

        # Replay the existing sequence
        for entry in currentGame:
            entry.show(.5)
            utime.sleep(.2)

        # Now they've got to enter each one.
        for entry in currentGame:
            inputPin = None

            # Loop until they enter something or 5 seconds elapses.
            start = utime.time()
            while inputPin is None and utime.time() - start < 5:
                for pin in pins:
                    # Are they pressing that button?
                    if pin.button.value() == 1:
                        print("Button %s was pushed..." % pin.name)
                        pin.buzz(True)
                        pin.led.on()
                        inputPin = pin
                        # Loop until they release the button.
                        while pin.button.value() == 1:
                            pass
                        pin.buzz(False)
                        pin.led.off()
                        print("...and released.")

            # They got it wrong! Or 5 seconds elapsed.
            if inputPin is not entry:
                print("They lost! Actual answer was %s." % entry.name)
                # Blink the correct one in their stupid face.
                entry.buzz(True)
                for _ in range(12):
                    entry.led.toggle();
                    utime.sleep(.1)
                entry.buzz(False)
                # Start a new game, brings us back to the outer while loop.
                lost = True
                break

            utime.sleep(1)

import machine
import utime
import random

# Since there's multiple things that are logically grouped together, lets logically group them together.
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

# Configure our program so it knows what pins go where.
# This is all centralized to keep it from being scattered all over the place.
pins = [
    SimonIO("Green", 16, 17, 262), # 262 = freq for C4 note. These notes are from the C major scale.
    SimonIO("Red", 18, 19, 294), # D4
    SimonIO("Yellow", 20, 21, 330), # E4
    SimonIO("Blue", 26, 27, 349), # F4
]

# Each one of these that's set to Vcc (positive) will increase the difficulty
difficultyPins = [
    machine.Pin(1, machine.Pin.IN, machine.Pin.PULL_DOWN),
    machine.Pin(2, machine.Pin.IN, machine.Pin.PULL_DOWN),
]

buzzer = machine.PWM(machine.Pin(28))

# There's no escape!
while True:
    # Reset game to initial state.
    print("Reset.")
    for pin in pins:
        pin.led.off()
    buzzer.duty_u16(0)
    currentGame = []

    # Run a sort of attract mode.
    print("Running attract.")
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

    # Each pin that's set reduces the amount of time throughout the program.
    difficulty = 1.0
    for difficultyPin in difficultyPins:
        difficulty -= .4 if difficultyPin.value() == 1 else 0

    # A base of 5 seconds.
    inputTime = 5.0 * difficulty

    print("Difficulty %f (inputTime: %f" % (difficulty, inputTime))

    while not lost:
        # Add a new entry.
        newPin = random.choice(pins)
        currentGame.append(newPin)
        print("Adding new entry %s." % newPin.name)

        # Replay the existing sequence
        for entry in currentGame:
            entry.show(.5 * difficulty)
            utime.sleep(.2 * difficulty)

        # Now they've got to enter that same sequence.
        for entry in currentGame:
            inputPin = None

            # Loop until they enter something or inputTime elapses.
            start = utime.time()
            while inputPin is None and utime.time() - start < inputTime:
                # Loop through the pins...
                for pin in pins:
                    # ...and check each ones button to see if it's pushed.
                    if pin.button.value() == 1:
                        print("Button %s was pushed..." % pin.name)
                        # Give the real human player feedback that the button was pushed.
                        pin.buzz(True)
                        pin.led.on()
                        inputPin = pin
                        # Loop until they release the button.
                        while pin.button.value() == 1:
                            pass
                        pin.buzz(False)
                        pin.led.off()
                        print("...and released.")
                        # Don't loop through the rest of the pins because we got a hit already.
                        break

            # They got it wrong! Or inputTime elapsed.
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

        # Add a bit of a delay so it doesn't jump right into the replay when they release the button
        utime.sleep(1)

import machine
import utime
import random

pressedPin = None

def button_handler(buttonPin):
    foundPin = None
    for pin in pins:
        if pin.button is buttonPin:
            foundPin = pin
            break

    if foundPin is None:
        print("Unable to find button?!")
        return

    if buttonPin.value() == 1:
        button_pressed_handler(foundPin)
    else:
        button_released_handler(foundPin)

def button_pressed_handler(pin):
    global pressedPin

    # Prevent weirdness when they push multiple buttons at once.
    if pressedPin is not None:
        print("They're already pushing %s!" % pressedPin.name)
        return

    pressedPin = pin
    print("User pressed %s" % pin.name)

def button_released_handler(pin):
    global pressedPin

    if pressedPin is None:
        print("They released a button but never pushed one?!")
        return

    # Prevent weirdness when they push multiple buttons at once.
    if pressedPin is not pin:
        print("They released %s but pushed %s!" % (pin.name, pressedPin.name))
        return

    pressedPin = None

class SimonIO:
    def __init__(self, name, ledPin, buttonPin, buzzerFreq):
        self.name = name
        self.led = machine.Pin(ledPin, machine.Pin.OUT)
        self.button = machine.Pin(buttonPin, machine.Pin.IN, machine.Pin.PULL_DOWN)
        self.button.irq(trigger=machine.Pin.IRQ_RISING | machine.Pin.IRQ_FALLING, handler=button_handler)
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

        # Now they've got to enter that same sequence.
        for entry in currentGame:
            inputPin = None

            # Loop until they enter something or 5 seconds elapses.
            start = utime.time()
            # This gets cleared out here. The IRQ handler will be called and
            # set it once something is entered, breaking our loop.
            print("Waiting for input...")
            while pressedPin is None and utime.time() - start < 5:
                pass

            # Store this because it's going to be overwritten by the IRQ handler
            wasPressed = pressedPin

            # If they pushed a button, give feedback and loop until they release it.
            if wasPressed is not None:
                wasPressed.led.on()
                wasPressed.buzz(True)
                while pressedPin is not None:
                    pass
                wasPressed.led.off()
                wasPressed.buzz(False)

            # They got it wrong! Or 5 seconds elapsed.
            if wasPressed is not entry:
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

        # Do a bit of a delay so it doesn't jump right into the replay when they release the button
        utime.sleep(1)

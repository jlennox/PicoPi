import machine
import utime
import random
import _thread

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


#   AAAAAAA
#   F     B
#   F     B
#   GGGGGGG
#   E     C
#   E     C
#   DDDDDDD

# Represents a single 7 segment numeric display. Each cell is a single pin.
# The above graphic represents the letter of each cell.

# Now lets represent multiple digits at once so we can show numbers larger than 9.
class Digits:
    def __init__(self, a, b, c, d, e, f, g, grounds):
        self.a = machine.Pin(a, machine.Pin.OUT)
        self.b = machine.Pin(b, machine.Pin.OUT)
        self.c = machine.Pin(c, machine.Pin.OUT)
        self.d = machine.Pin(d, machine.Pin.OUT)
        self.e = machine.Pin(e, machine.Pin.OUT)
        self.f = machine.Pin(f, machine.Pin.OUT)
        self.g = machine.Pin(g, machine.Pin.OUT)
        self.grounds = list(map(lambda gr: machine.Pin(gr, machine.Pin.OUT), grounds))

    def debug(self):
        self.displayDigit(8)
        for gr in self.grounds:
            gr.value(1)

    def display(self, num):
        index = 0

        # Disable all of the grounds so that any existing value isn't ghosted onto
        # this segment.
        for i in range(len(self.grounds)):
            self.grounds[i].value(0)

        # Go number by number.
        for digitStr in str(num):
            # Maybe they're really good and got a 3 digit score! Better not crash.
            # This will set the proper cells to enabled.
            if index < len(self.grounds):
                self.displayDigit(int(digitStr))

            # Now we need to re-enable only the proper ground.
            for i in range(len(self.grounds)):
                self.grounds[i].value(1 if i == index else 0)

            index += 1

            # This sleep is load bearing. The segment needs a bit of time to actually
            # display its value.
            utime.sleep(.001)

    def clear(self):
        self.setCells()

    def setCells(self, a=0, b=0, c=0, d=0, e=0, f=0, g=0):
        self.a.value(a)
        self.b.value(b)
        self.c.value(c)
        self.d.value(d)
        self.e.value(e)
        self.f.value(f)
        self.g.value(g)

    # This is kind of a sloppy but an easy way to do this.
    def displayDigit(self, num):
        if num == 1:   self.setCells(b=1, c=1)
        elif num == 2: self.setCells(a=1, b=1, g=1, e=1, d=1)
        elif num == 3: self.setCells(a=1, b=1, g=1, c=1, d=1)
        elif num == 4: self.setCells(f=1, g=1, b=1, c=1)
        elif num == 5: self.setCells(a=1, f=1, g=1, c=1, d=1)
        elif num == 6: self.setCells(a=1, f=1, g=1, e=1, c=1, d=1)
        elif num == 7: self.setCells(a=1, b=1, c=1)
        elif num == 8: self.setCells(a=1, b=1, c=1, d=1, e=1, f=1, g=1)
        elif num == 9: self.setCells(a=1, b=1, c=1, d=1, f=1, g=1)
        elif num == 0: self.setCells(a=1, b=1, c=1, d=1, e=1, f=1)

currentDigit = 0

def updateDisplay():
    digits.display(currentDigit)

def updateDisplayAndSleep():
    for _ in range(7):
        digits.display(currentDigit)
        utime.sleep(.01)

def digitRefreshThread():
    global digits
    global currentDigit
    while currentDigit != -1:
        updateDisplay()
        utime.sleep(.01)

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

digits = Digits(13, 7, 8, 10, 9, 12, 15, [6, 11])

#_thread.start_new_thread(digitRefreshThread, ())

highScore = 0
with open("highscore.txt", "r") as highScoreFile:
    highScoreStr = highScoreFile.read()
    highScore = int("0" if highScoreStr == "" or highScoreStr is None else highScoreStr)

print("Found highscore: " + str(highScore))

# There's no escape!
while True:
    # Reset game to initial state.
    print("Reset.")
    for pin in pins:
        pin.led.off()
    buzzer.duty_u16(0)
    currentGame = []
    currentScore = 0

    # Run a sort of attract mode and show highscore.
    currentDigit = highScore

    print("Running attract.")
    for _ in range(2):
        for pin in pins:
            pin.led.on()
            pin.buzz(True)
            updateDisplayAndSleep()

        for pin in reversed(pins):
            pin.led.off()
            pin.buzz(False)
            updateDisplayAndSleep()

    for _ in range(4):
        for pin in pins:
            pin.led.toggle()
        updateDisplayAndSleep()

    lost = False

    # Each pin that's set reduces the amount of time throughout the program.
    difficulty = 1.0
    for difficultyPin in difficultyPins:
        difficulty -= .4 if difficultyPin.value() == 1 else 0

    # A base of 5 seconds.
    inputTime = 5.0 * difficulty

    print("Difficulty %f (inputTime: %f" % (difficulty, inputTime))

    while not lost:
        # Show their current score as they play
        currentDigit = currentScore
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
                updateDisplay()
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
                        # Sleep for a small amount of time. After testing, sometimes the button
                        # would register again.
                        utime.sleep(.2)
                        # Don't loop through the rest of the pins because we got a hit already.
                        break
                utime.sleep(.01)

            digits.clear()

            # They got it wrong! Or inputTime elapsed.
            if inputPin is not entry:
                print("They lost! Actual answer was %s." % entry.name)
                # Blink the correct one in their stupid face.
                entry.buzz(True)
                for _ in range(12):
                    entry.led.toggle();
                    updateDisplayAndSleep()
                entry.buzz(False)
                # Start a new game, brings us back to the outer while loop.
                lost = True
                break

        # Add a bit of a delay so it doesn't jump right into the replay when they release the button
        utime.sleep(1)

        currentScore += 1
        if currentScore > highScore:
            print("New highscore! " + str(currentScore))
            highScore = currentScore
            with open("highscore.txt", "w") as highScoreFile:
                highScoreFile.write(str(highScore))

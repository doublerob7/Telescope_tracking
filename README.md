# Telescope_tracking

Auto-track the sky with your dobsonian telescope! Preserve the manual exploration of the night sky without having to continuously fine-adjust for high magnification targets. Based on Arduino Uno, this sketch is intended to track the current position when the telescope slewed to any position in the sky by keypad. This means no GO-TO, no object catalogue, just assisted manual viewing.

The goal of this repo is to eventually contain all the sketches, schematics, CAD files and parts list for building your own DIY tracker for a dobsonian telescope.

## Parts list
Electronics

1. Arduino Uno
2. Adafruit Motor shield V2
3. 2x Stepper motors (200 steps /rev by default)
4. 12 key Keypad
5. 16x2 LCD display
6. Battery for steppers (+battery for Arduino, if different enough)

Mechanical

1. Telescope
2. 2x Worm gear, 150:1 (see CAD files, coming...)
3. Stepper motor mounting hardware
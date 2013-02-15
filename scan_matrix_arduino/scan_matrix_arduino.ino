/*
 * Use a 5×7 LED matrix, part number A3570E or A-3570E, as a light
 * sensor on an Arduino.  The intended use here is scanning punched
 * cards with 4.57mm hole spacing, and we only scan a couple of times
 * a second, then sending the data back to the host via USB.
 *
 * Note that this circuit is very sensitive to noise on the power
 * supply.  I can't use it while the Arduino is plugged into my
 * netbook when the netbook is plugged into the wall.  It's also very
 * sensitive to parasitic capacitances and static electricity in the
 * environment.
 *
 * Wiring table:
 * Arduino pin:  A-3570E pin:
 * 2             9
 * 3             14
 * 4             8
 * 5             12
 * 6             1
 * 7             7
 * 8             2
 *
 * 9             13
 * 10            3
 * 11            11
 * 12            10
 * 13            6
 */

int row_pins[] = { 2, 3, 4, 5, 6, 7, 8 };
int nrows = sizeof(row_pins) / sizeof(*row_pins);
int column_pins[] = { 9, 10, 11, 12, 13 };
int ncols = sizeof(column_pins) / sizeof(*column_pins);

void setup() {
  // say hello to show that we're here:
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
 
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
  delay(100);
  
  Serial.begin(9600);
  Serial.write("hello\n");
}

typedef unsigned char column;

void loop() {
/*digitalWrite(13, HIGH);
  delay(10);
  digitalWrite(13, LOW);
*/
  unsigned char illuminated[ncols];
  for (int i = 0; i < ncols; i++) {
    illuminated[i] = scan_column(column_pins[i]);
  }
  for (int i = 0; i < ncols; i++) {
    Serial.print(illuminated[i], HEX);
    Serial.write(" ");
  }
  Serial.write("\n");

  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < ncols; j++) {
      display_column(column_pins[j], illuminated[j]);
    }
  }
}

void display_column(char cathode_pin, unsigned char leds) {
  pinMode(cathode_pin, OUTPUT);
  digitalWrite(cathode_pin, 1);

  for (int i = 0; i < nrows; i++) {
    pinMode(row_pins[i], OUTPUT);
    digitalWrite(row_pins[i], !!(leds & 1 << i));
  }

  delay(1);
  
  pinMode(cathode_pin, INPUT);
  digitalWrite(cathode_pin, 0); // disable pullup
}

unsigned char scan_column(char cathode_pin) {
  pinMode(cathode_pin, OUTPUT);
  digitalWrite(cathode_pin, 0); // Reverse-bias the LEDs to use them as photodiodes.

  for (int i = 0; i < nrows; i++) {
    pinMode(row_pins[i], OUTPUT);
    digitalWrite(row_pins[i], 1);
  }

  delay(1);

  for (int i = 0; i < nrows; i++) {
    pinMode(row_pins[i], INPUT);
    digitalWrite(row_pins[i], 0); // disable pullup
  }
  
  int pin_state[nrows];
  int pin_time[nrows];
  int pins_left = nrows;
  for (int i = 0; i < nrows; i++) {
    pin_time[i] = -1;
    pin_state[i] = 1;
  }
  
  for (int t = 0; t < 1000 && pins_left; t++) {
    for (int i = 0; i < nrows; i++) {
      if (pin_state[i] && !digitalRead(row_pins[i])) {
        pin_state[i] = 0;
        pin_time[i] = t;
        pins_left--;
      }
    }
  }

/* * /
  if (cathode_pin == 9) Serial.write("\n");
  for (int i = 0; i < nrows; i++) {
    Serial.print(i, DEC);
    Serial.write(" ");
    Serial.print(pin_time[i], DEC);
    Serial.write(" ");
  }
  Serial.write("\n");

// */

  pinMode(cathode_pin, INPUT);
  digitalWrite(cathode_pin, 0); // disable pullup

  unsigned char rv = 0;
  for (signed char i = nrows-1; i >= 0; --i) {
    rv <<= 1;
    if (pin_time[i] == -1) rv++;
  }

  return rv;
}

const char VERSION[] = "0.1";
const int BAUD_RATE = 115200;

const int SYNC_PIN = 6;
const int STATUS_PIN = 5;
const int POWER_PIN = 4;
const int PLUS_PIN = 3;
const int PAUSE_PIN = 2;

const int N_BUTTONS = 3;
const int POWER_BUTTON = 0;
const int PLUS_BUTTON = 1;
const int PAUSE_BUTTON = 2;
const int BUTTON_AS_PIN[] = {POWER_PIN, PLUS_PIN, PAUSE_PIN};
#define button_to_pin(x) BUTTON_AS_PIN[x]

const int BUTTON_PRESS_MS = 10; // Time to "press" a button, in milliseconds
const int BUTTON_LONG_PRESS_MS = 5200; // Time to "long press" a button, in milliseconds

const int SYNC_PULSE_MIN_MS = 1000;
const int SYNC_PULSE_MAX_MS = 5000;

unsigned int button_state = 0;
unsigned long button_release_deadline[3];

typedef bool CommandDispatcher(void);

// Report with an identifying string & version information
bool cmd_identify();
const char IDENTIFICATION[] = "TENS synchronization v";

// Report the current value of millis()
bool cmd_get_clock();
// Turn on the TENS unit
// Activates the power button for BUTTON_PRESS_MS
bool cmd_tens_on();
// Turn off the TENS unit
// Activates the power button for BUTTON_LONG_PRESS_MS
bool cmd_tens_off();
// Check whether the TENS unit is on or off
bool cmd_tens_status();
// Initialize the synchronization procedure
bool cmd_begin_sync();
// Check if the synchronization pulse was detected
bool cmd_check_sync();
// Enter "keep awake" mode for the TENS
// This "pokes" the TENS by activating the power button briefly at regular
// intervals

// Not exactly the most memory efficient implementation but we should be okay
/*const CommandDispatcher *const COMMAND_TABLE[255] = {
  ['i'] = *cmd_identify,
  ['t'] = *cmd_get_clock,
  ['s'] = *cmd_begin_sync,
  ['S'] = *cmd_check_sync
};*/
CommandDispatcher * command_table[255] = {};

unsigned long loop_start_time = 0;
unsigned long sync_start_time = 0;
unsigned long sync_detection_time;
bool sync_state;

void press_button(int button, int duration);
void release_button(int button);
inline bool is_time_to_release_button(int button) {
  return (button_state & (1<<button))
      && loop_start_time >= button_release_deadline[button];
}

void setup() {
	command_table['i'] = *cmd_identify;
  command_table['t'] = *cmd_get_clock;
  command_table['s'] = *cmd_begin_sync;
  command_table['S'] = *cmd_check_sync;
  command_table['n'] = *cmd_tens_on;
  command_table['f'] = *cmd_tens_off;
  command_table['p'] = *cmd_tens_status;
  Serial.begin(115200);
  pinMode(SYNC_PIN, INPUT);
  pinMode(STATUS_PIN, INPUT);
  pinMode(POWER_PIN, OUTPUT);
  pinMode(PLUS_PIN, OUTPUT);
  pinMode(PAUSE_PIN, OUTPUT);
  sync_state = digitalRead(SYNC_PIN);
  sync_detection_time = 0;
  //cmd_identify();
}

void loop() {
  loop_start_time = millis();

  /* Check for commands */
  if (Serial.available()) {
    char command = Serial.read();
    // We received a command byte: execute the associated handler, if there is one
    if (command_table[command]) {
      command_table[command]();
    }
  }
  /* Check for button "release" deadlines */
  for (int btn = 0; btn < N_BUTTONS; ++btn) {
    if (is_time_to_release_button(btn))
      release_button(btn);
  }
  /* Handle things required only if synchronization has been initiated */
  if (sync_start_time) {
    /* Check for sync pulse detection */
    int new_state = digitalRead(SYNC_PIN);
    if (new_state && !sync_state) {
      sync_detection_time = millis();
    }
    sync_state = new_state;
    /* Check for sync pulse deactivation deadlines */
    if (
      // Maximum duration has elapsed since sync was initiated
      (loop_start_time >= sync_start_time + SYNC_PULSE_MAX_MS)
      ||
      // Minimum duration has elapsed since sync was detected
      (loop_start_time >= sync_detection_time + SYNC_PULSE_MIN_MS)
    ) {
      press_button(POWER_BUTTON, BUTTON_PRESS_MS);
      sync_start_time = 0;
    }
  }
}

bool cmd_identify() {
  Serial.print(IDENTIFICATION);
  Serial.print(VERSION);
  Serial.println();
  return true;
}

bool cmd_get_clock() {
  Serial.write(loop_start_time);
  return true;
}

bool cmd_tens_on() {
  press_button(POWER_BUTTON, BUTTON_PRESS_MS);
  return true;
}

bool cmd_tens_off() {
  press_button(POWER_BUTTON, BUTTON_LONG_PRESS_MS);
  return true;
}

bool cmd_tens_status() {
  Serial.write(bool(digitalRead(STATUS_PIN)));
  return true;
}

bool cmd_begin_sync() {
  press_button(PLUS_BUTTON, BUTTON_PRESS_MS);
  sync_start_time = millis();
  sync_detection_time = 0;
  sync_state = 0;
  return true;
}

bool cmd_check_sync() {
  Serial.write(sync_detection_time);
  return true;
}

void press_button(int button, int duration) {
  digitalWrite(button_to_pin(button), HIGH);
  button_release_deadline[button] = millis() + duration;
  button_state |= 1<<button;
}

void release_button(int button) {
  digitalWrite(button_to_pin(button), LOW);
  button_state &= ~(1<<button);
}

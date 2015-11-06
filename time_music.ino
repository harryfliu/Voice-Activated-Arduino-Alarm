#include <WaveHC.h>
#include <WaveUtil.h>
#include <Wire.h>
#include "RTClib.h"

SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the volumes root directory
FatReader f;
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

//uint8_t dirLevel; // indent level for file/dir names    (for prettyprinting)
//dir_t dirBuf;     // buffer for directory reads

RTC_DS1307 rtc;
unsigned int deactivate = 0;
unsigned int goneThru = 0;
unsigned int alarmOn = 0; // if alarm is activated go to 1, then reset after loop
int BUTTON = 0;
int button_pressed = 0;
/*
 * Define macro to put error messages in flash memory
 */
#define error(msg)

// Function definitions (we define them here, but the code is below)
//void play(FatReader &dir);

void setup() {
  pinMode(15, INPUT);
  Serial.begin(57600);
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  rtc.begin();

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  
  putstring_nl("\nWave test!");  // say we woke up!
    
  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(FreeRam());
  
    // Set the output pins for the DAC control. This pins are defined in the library
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  
  //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    error("Card init. failed!");  // Something went wrong, lets print out why
  }
  
  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);
  
  // Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {   // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                           // we found one, lets bail
  }
  if (part == 5) {                     // if we ended up not finding one  :(
    error("No valid FAT partition!");  // Something went wrong, lets print out why
  }
  
  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(), DEC);     // FAT16 or FAT32?
  
  // Try to open the root directory
  if (!root.openRoot(vol)) {
    error("Can't open root dir!");      // Something went wrong,
  }
  
  // Whew! We got past the tough parts.
  putstring_nl("Files found (* = fragmented):");
  
  // Print out all of the files in all the directories.
  root.ls(LS_R | LS_FLAG_FRAGMENTED);
    
}

void loop() {
  DateTime now = rtc.now();
  int hr = now.hour();
  int minu = now.minute();
  root.rewind();
  signed int peakToPeak = 0;   // peak-to-peak level
  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;
  unsigned int sample;
  unsigned long startMillis= millis();  // Start of sample window
  if (hr == 8 && minu == 0 && deactivate == 0 && goneThru == 0){
    playcomplete("PRMT.WAV");
    //Serial.print("Out This is the current sample window: ");
    //Serial.print(millis() - startMillis, DEC);
    //Serial.println();
    while (millis() - startMillis < 20000){
      //Serial.print("In This is the current sample window: ");
      //Serial.print(millis() - startMillis, DEC);
      //Serial.println();
      Serial.print("\n\n\n\n\nShout right now!");
      sample = analogRead(0);
      Serial.println();
      Serial.print(sample);
      Serial.println();
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
      peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
      double volts = (peakToPeak * 3.3) / 1024;  // convert to volts
      if (volts > 2)
      {
        Serial.print(volts);
        Serial.println();
        Serial.print(now.year(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.day(), DEC);
        Serial.print(' ');
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.println();
        Serial.print("\n\n\n\n\n\n\n\n\n\n");
        Serial.print("This is working fine! You have deactivated the alarm.");
        Serial.print("\n\n\n\n\n\n\n\n\n\n");
        Serial.println();
        deactivate = 1;
        alarmOn = 0;
        break;
      }
      else if (volts <= 2){
        Serial.print("\n\n\n\n\n\n\n\n\n\n");
        Serial.print("Did not shout loudly enough.");
        Serial.print("\n\n\n\n\n\n\n\n\n\n");
        alarmOn = 1;
        deactivate = 0;
      }
    }
    if(alarmOn){
      playcomplete("ALRM.WAV");
      deactivate = 1;
    }
    if(deactivate){
      playcomplete("ALRMOFF.WAV");
    }
    Serial.print("It's not time for the alarm yet.");
    Serial.println();
    Serial.print(now.month());
    Serial.print('/');
    Serial.print(now.day());
    Serial.println();
    Serial.print(now.hour());
    Serial.print(':');
    Serial.print(now.minute());
    Serial.print(':');
    Serial.print(now.second());
    Serial.println();
    delay(1000);
    goneThru = 1;
  }
  else{
    goneThru = 0;
    if (hr == 21 && minu == 59){
      deactivate = 0;
    }
    Serial.print("It's not time for the alarm yet.");
    Serial.println();
    Serial.print(now.month());
    Serial.print('/');
    Serial.print(now.day());
    Serial.println();
    Serial.print(now.hour());
    Serial.print(':');
    Serial.print(now.minute());
    Serial.print(':');
    Serial.print(now.second());
    Serial.println();
    delay(1000);
  }
}

/////////////////////////////////// HELPERS
/*
 * print error message and halt
 */
/*
 * print error message and halt if SD I/O error, great for debugging!
 */
void sdErrorCheck(void) {
  if (!card.errorCode()) return;
  PgmPrint("\r\nSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  PgmPrint(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}

// Plays a full file from beginning to end with no pause.
void playcomplete(char *name) {
  // call our helper to find and play this name
  playfile(name);
  while (wave.isplaying) {
    BUTTON = digitalRead(15);
    if(BUTTON == HIGH){
      button_pressed = 1;
      Serial.print(BUTTON, DEC);
      wave.stop();
    }
    else{
      Serial.print(BUTTON, DEC);
      Serial.print("The button is low currently!\n");
    }
  }
  // now its done playing
}

void playfile(char *name) {
  // see if the wave object is currently doing something
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  // look in the root directory and open the file
  if (!f.open(root, name)) {
    putstring("Couldn't open file "); Serial.print(name); return;
  }
  // OK read the file and turn it into a wave object
  if (!wave.create(f)) {
    putstring_nl("Not a valid WAV"); return;
  }
  
  // ok time to play! start playback
  wave.play();
}
/*
 * play recursively - possible stack overflow if subdirectories too nested
 */
/*void play(FatReader &dir) {
  FatReader file;
  while (dir.readDir(dirBuf) > 0) {    // Read every file in the directory one at a time
  
    // Skip it if not a subdirectory and not a .WAV file
    if (!DIR_IS_SUBDIR(dirBuf)
         && strncmp_P((char *)&dirBuf.name[8], PSTR("WAV"), 3)) {
      continue;
    }

    Serial.println();            // clear out a new line
    
    for (uint8_t i = 0; i < dirLevel; i++) {
       Serial.write(' ');       // this is for prettyprinting, put spaces in front
    }
    if (!file.open(vol, dirBuf)) {        // open the file in the directory
      error("file.open failed");          // something went wrong
    }
    
    if (file.isDir()) {                   // check if we opened a new directory
      putstring("Subdir: ");
      printEntryName(dirBuf);
      Serial.println();
      dirLevel += 2;                      // add more spaces
      // play files in subdirectory
      play(file);                         // recursive!
      dirLevel -= 2;    
    }
    else {
      // Aha! we found a file that isnt a directory
      putstring("Playing ");
      printEntryName(dirBuf); // print it out
      if (!wave.create(file)) {            // Figure out, is it a WAV proper?
        putstring(" Not a valid WAV");     // ok skip it
      } 
      else {
        Serial.println();                  // Hooray it IS a WAV proper!
        wave.play();                       // make some noise!
        
        uint8_t n = 0;
        while (wave.isplaying) {// playing occurs in interrupts, so we print dots in realtime
          putstring(".");
          if (!(++n % 32))Serial.println();
          delay(100);
        }       
        sdErrorCheck();                    // everything OK?
        // if (wave.errors)Serial.println(wave.errors);     // wave decoding errors
      }
    }
  }
}*/

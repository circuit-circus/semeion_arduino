import processing.serial.*;

Serial myPort;  // Create object from Serial class
String val = "";      // Data received from the serial port
String lastVal = "";
color c = color(0, 0, 0);
void setup() {
  size(800, 600);
  String portName = Serial.list()[1];
  myPort = new Serial(this, portName, 9600);
  textSize(32);
}

void draw(){
  if ( myPort.available() > 0) {  // If data is available,
    val = trim(myPort.readStringUntil('\n'));         // read it and store it in val
    println(val);
  }
    
  if(val != null && !val.equals(lastVal)) {
    
    lastVal = val;
    
    background(c);
    
    switch(val) {
      case "IDLE": 
        c = color(0);
        break;
      case "DU": 
        c = color(173, 104, 104);
        break;
      case "DP": 
        c = color(105, 173, 130);
        break;
      case "AP": 
        c = color(14, 234, 95);
        break;
      case "AU": 
        c = color(232, 22, 11);
        break;
      default:
        text(val, 10, 40);
        break;
    }
    
    
    
  }
  
}

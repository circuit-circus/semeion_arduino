import processing.serial.*;

Serial myPort;  // Create object from Serial class

int val;      // Data received from the serial port
int lastVal;

int high;
int low;

color c = color(0, 0, 0);

String[] list;

void setup() {
  size(800, 600);
  String portName = Serial.list()[1];
  myPort = new Serial(this, portName, 9600);
  textSize(160);
  
  list = new String[3];
  list[0] = "0";
  list[1] = "0";
  list[2] = "0";
}

void draw(){
  if ( myPort.available() > 0) {  // If data is available,
    String portVal = trim(myPort.readStringUntil('\n'));         // read it and store it in val
    list = split(portVal, ',');
    
    if(list != null && list.length > 2) {
      String sVal = list[0];
      String sLow = list[1];
      String sHigh = list[2];
      
      val = int(sVal);
      low = int(sLow);
      high = int(sHigh);
    }
  }
    
  if(val != lastVal) {
    
    lastVal = val;
    
    println(val);
    
    float col = map(val, low, high, 0, 255);
    c = color(int(col), int(col), int(col));
    background(c);
    
    float num = map(val, low, high, 0, 100);
    
    text((int)num, 200, 200);
    
    //switch(val) {
    //  case "IDLE": 
    //    c = color(0);
    //    break;
    //  case "DU": 
    //    c = color(173, 104, 104);
    //    break;
    //  case "DP": 
    //    c = color(105, 173, 130);
    //    break;
    //  case "AP": 
    //    c = color(14, 234, 95);
    //    break;
    //  case "AU": 
    //    c = color(232, 22, 11);
    //    break;
    //  default:
    //    text(val, 200, 200);
    //    break;
    //}
    
    
    
  }
  
}

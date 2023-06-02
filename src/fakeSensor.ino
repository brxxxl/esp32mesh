#include "painlessMesh.h"
#include <Arduino_JSON.h> // original
//#include "Arduino_JSON.h" // testado e tbem nao funciona

#define MSGS
#define DEBUG
//#define DEBUGMSG

// MESH Details
#define   MESH_PREFIX     "RNTMESH" //name for your MESH
#define   MESH_PASSWORD   "MESHpassword" //password for your MESH
#define   MESH_PORT       5555 //default port

//String to send to other nodes with sensor readings
String readings;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings

//Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

//Number for this node
uint32_t nodeNumber = 1;
unsigned int nid = 0;

unsigned long start_time; 
unsigned long timed_event;
unsigned long current_time;

long media (int size, int pin) {
	long vec[size];
	long val = 0;

	for (int i = 0; i < size; i++){
		vec[i] = analogRead(pin);
	}

	for (int i = 0; i < size; i++){
		val += vec[i];
	}
	val = val/size;
	return val;
}

String getReadings () {
	JSONVar jsonReadings;
	jsonReadings["node"] = nodeNumber;
	jsonReadings["temp"] = media(25,36);
	jsonReadings["hum"] = media(25,39);
	jsonReadings["pres"] = random(0,100);
	readings = JSON.stringify(jsonReadings);
	return readings;
}

void sendMessage () {
	String msg = getReadings();
	mesh.sendBroadcast(msg);
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
	#ifdef DEBUG
		Serial.printf("Received from %u: %s\n", from, msg.c_str());
	#endif
	JSONVar myObject = JSON.parse(msg.c_str());
	int node = myObject["node"];
	int temp = myObject["temp"];
	int hum = myObject["hum"];
	int pres = myObject["pres"];
}

void newConnectionCallback(uint32_t nodeId) {
	#ifdef MSGS
		Serial.printf("New Connection, nodeId = %u\n", nodeId);
	#endif
}

void changedConnectionCallback() {
	#ifdef MSGS
		Serial.printf("Changed connections\n");
	#endif
}

void nodeTimeAdjustedCallback(int32_t offset) {
	#ifdef MSGS
		Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
	#endif
}

void setup() {

	timed_event = 5000;
	current_time = millis();
	start_time = current_time;
	
	Serial.begin(9600);

	#ifdef DEBUGMSG
		mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
	#endif
	#ifndef DEBUGMSG
		mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
	#endif

	mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
	mesh.onReceive(&receivedCallback);
	mesh.onNewConnection(&newConnectionCallback);
	mesh.onChangedConnections(&changedConnectionCallback);
	mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

	userScheduler.addTask(taskSendMessage);
	taskSendMessage.enable();
	nid = mesh.getNodeId();
}

void loop() {
	getReadings();
	
	current_time = millis();
	if (current_time - start_time >= timed_event) {
		Serial.printf("Sent by this node(%u): ",nid);
		Serial.print(readings);
		Serial.println();
		start_time = current_time;
	}
	mesh.update();
}

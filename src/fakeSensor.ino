#include "painlessMesh.h" // Instalar essa biblioteca pelo PlatformIO/ArduinoIDE
#include <Arduino_JSON.h> // Instalar essa biblioteca pelo PlatformIO/ArduinoIDE (na IDE vem por padrão a ArduinoJSON SEM UNDERLINE)
// Instalar a TaskScheduler.h pelo PlatformIO/ArduinoIDE
// Instalar a AsyncTCP.h pelo PlatformIO/ArduinoIDE

#define MSGS		// Para imprimir as mensagens de recepção e envio de dados
#define DEBUG		// Para inicializar no modo debug, imprimindo as mensagens de inicialização e de alteração de conexões
//#define DEBUGMSG	// Para imprimir as mensagens de debug da biblioteca painlessMesh

// MESH Details
#define MESH_PREFIX "RNTMESH"			// Nome da rede MESH
#define MESH_PASSWORD "MESHpassword"	// Senha para rede MESH
#define MESH_PORT 5555					// Porta padrão

String readings; // String com as leituras dos sensores

Scheduler userScheduler; // Para o controle da task principal
painlessMesh mesh;

// User stub
void sendMessage();		// Protótipo para que o PlatformIO não reclame
String getReadings();	// Protótipo para que o PlatformIO não reclame

// Cria as tasks para enviar mensagens e fazer as medições a cada 5 segundos
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

// Identificação do node
uint32_t nodeNumber = 1;	// O número arbitrário deste node na rede MESH
unsigned int nid = 0;		// O ID deste node na rede MESH, gerado pelo painlessMesh

unsigned long start_time;
unsigned long timed_event;
unsigned long current_time;

// Calcula a média de leituras de um sensor analógico no pino definido pelo usuário
long media (int size, int pin) {
	long val = 0;	// Variável para armazenar a soma das leituras e calcular a média

	for (int i = 0; i < size; i++){
		val += analogRead(pin);
	}

	val = val/size;
	return val;
}

// Retorna as leituras dos sensores em formato JSON
String getReadings () {
	JSONVar jsonReadings;
	jsonReadings["node"] = nodeNumber;
	jsonReadings["temp"] = media(25,36);
	jsonReadings["hum"] = media(25,39);
	jsonReadings["pres"] = random(0,100);
	readings = JSON.stringify(jsonReadings);
	return readings;
}

// Realiza e envia as leituras dos sensores para a rede MESH
void sendMessage () {
	String msg = getReadings();
	mesh.sendBroadcast(msg);
}

// Imprime no serial as mensagens enviadas pela rede MESH
void receivedCallback( uint32_t from, String &msg ) {
	#ifdef MSGS
		Serial.printf("Received from %u: %s\n", from, msg.c_str());
	#endif
	JSONVar myObject = JSON.parse(msg.c_str());
	int node = myObject["node"];
	int temp = myObject["temp"];
	int hum = myObject["hum"];
	int pres = myObject["pres"];
}

// Callbacks de nova conexão para debug
void newConnectionCallback(uint32_t nodeId) {
	#ifdef DEBUG
		Serial.printf("New Connection, nodeId = %u\n", nodeId);
	#endif
}

// Callbacks de alteração de conexão para debug
void changedConnectionCallback() {
	#ifdef DEBUG
		Serial.printf("Changed connections\n");
	#endif
}

// Callbacks de ajuste de tempo para debug
void nodeTimeAdjustedCallback(int32_t offset) {
	#ifdef DEBUG
		Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
	#endif
}

// Inicializa o node
void setup() {
	timed_event = 5000;
	current_time = millis();
	start_time = current_time;
	Serial.begin(9600);

	// Definição das mensagens de debug da biblioteca painlessMesh
	#ifdef DEBUGMSG
		// Liga todas as mensagens de debug
		mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE );
	#endif
	#ifndef DEBUGMSG
		// Liga apenas as mensagens de erro e de inicialização
		mesh.setDebugMsgTypes( ERROR | STARTUP );
	#endif

	mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );	// Inicialização da rede MESH
	mesh.onReceive(&receivedCallback);									// Callback para receber mensagens
	mesh.onNewConnection(&newConnectionCallback);						// Callback para nova conexão
	mesh.onChangedConnections(&changedConnectionCallback);				// Callback para alteração de conexão
	mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);					// Callback para ajuste de tempo

	userScheduler.addTask(taskSendMessage);	// Adiciona a task de envio de mensagens a cada 5 segundos
	taskSendMessage.enable();				// Habilita a task de envio de mensagens
	nid = mesh.getNodeId();					// Pega o ID deste node na rede MESH
}

// Loop principal
void loop() {
	mesh.update();				// Atualiza a rede MESH
	current_time = millis();	// Define o tempo atual
	if (current_time - start_time >= timed_event) {
		Serial.printf("Sent by this node(%u): ",nid);	// Imprime no serial o ID deste node
		Serial.print(readings);							// Imprime no serial as leituras dos sensores
		Serial.println();
		start_time = current_time;	// Atualiza o tempo inicial
	}
}
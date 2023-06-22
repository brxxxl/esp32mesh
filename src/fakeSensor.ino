#include "painlessMesh.h" // Instalar essa biblioteca pelo PlatformIO/ArduinoIDE
#include <Arduino_JSON.h> // Instalar essa biblioteca pelo PlatformIO/ArduinoIDE (na IDE vem por padrão a ArduinoJSON SEM UNDERLINE)
// Instalar a TaskScheduler.h pelo PlatformIO/ArduinoIDE
// Instalar a AsyncTCP.h pelo PlatformIO/ArduinoIDE

#define MSGS	// Para imprimir as mensagens de recepção e envio de dados
#define DEBUG	// Para inicializar no modo debug, imprimindo as mensagens de inicialização e de alteração de conexões
//#define DEBUGMSG	// Para imprimir as mensagens de debug da biblioteca painlessMesh

// Definição dos pinos dos sensores e número de leituras para fazer a média
#define num_leituras_media 30	// Número de leituras para fazer a média

// Identificação do node
uint32_t nodeNumber = 1;	// O número arbitrário deste node na rede MESH
unsigned int nid = 0;		// O ID deste node na rede MESH, gerado pelo painlessMesh

// MESH Details
#define MESH_PREFIX "RNTMESH"     // Nome da rede MESH
#define MESH_PASSWORD "MESHpassword"  // Senha para rede MESH
#define MESH_PORT 5555          // Porta padrão

String readings;	// String com as leituras dos sensores

Scheduler userScheduler;	// Para o controle da task principal
painlessMesh mesh;

// User stub
void sendMessage();		// Protótipo para que o PlatformIO não reclame
String getReadings();	// Protótipo para que o PlatformIO não reclame

// Cria as tasks para enviar mensagens e fazer as medições a cada 5 segundos
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

// Para o controle do tempo
unsigned long start_time;
unsigned long timed_event;
unsigned long current_time;

int node_loc, ch1_loc, ch2_loc, ch3_loc, ch4_loc;	// Local
int node_rec, ch1_rec, ch2_rec, ch3_rec;			// Recebido

// ---------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------

void setup() {
	timed_event = 5000;
	current_time = millis();
	start_time = current_time;
	Serial.begin(9600);

	// Definição das mensagens de debug da biblioteca painlessMesh
	#ifdef DEBUGMSG    // Liga todas as mensagens de debug
	mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE );
	#endif
	
	#ifndef DEBUGMSG    // Liga apenas as mensagens de erro e de inicialização
	mesh.setDebugMsgTypes( ERROR | STARTUP );
	#endif

	mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT ); // Inicialização da rede MESH
	mesh.onReceive(&receivedCallback);                  // Callback para receber mensagens
	mesh.onNewConnection(&newConnectionCallback);           // Callback para nova conexão
	mesh.onChangedConnections(&changedConnectionCallback);        // Callback para alteração de conexão
	mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);         // Callback para ajuste de tempo

	userScheduler.addTask(taskSendMessage); // Adiciona a task de envio de mensagens a cada 5 segundos
	taskSendMessage.enable();       // Habilita a task de envio de mensagens
	nid = mesh.getNodeId();         // Pega o ID deste node na rede MESH
}

//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------

void loop() {
	mesh.update();            
	current_time = millis();  

	ch1_loc = Leitura_Tensao_Canal(36, num_leituras_media)*1000.0; // local, em mV
	ch2_loc = Leitura_Tensao_Canal(39, num_leituras_media)*1000.0; 
	ch3_loc = Leitura_Tensao_Canal(34, num_leituras_media)*1000.0; 

	/*  ch1_loc = 11;
		ch2_loc = 12;
		ch3_loc = 13;
	*/

	/*  ch1_loc = 111;
		ch2_loc = 222;
		ch3_loc = 333;
	*/ 
	
	if (current_time - start_time >= timed_event) { // A cada 5 segundos
		Serial.printf("Sent by this node(%u): ",nid);	// Imprime no serial o ID deste node
		Serial.print(readings);	// Imprime no serial as leituras dos sensores
		Serial.println();

		start_time = current_time;  // Atualiza o tempo inicial
	}
	
}

//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
									 
void sendMessage() {	// Envia as leituras dos sensores para a rede MESH
	String msg;
	JSONVar jsonReadings;
	
	jsonReadings["node"] = nodeNumber;	// Cada valor está associado a um nome (tem que ser igual o que envia e recebe)
	jsonReadings["ch1"] = ch1_loc;
	jsonReadings["ch2"] = ch2_loc;
	jsonReadings["ch3"] = ch3_loc;

	Serial.print(" node_loc= "); Serial.print(nodeNumber);
	Serial.print("   ch1_loc= "); Serial.print(ch1_loc);
	Serial.print("   ch2_loc= "); Serial.print(ch2_loc);
	Serial.print("   ch3_loc= "); Serial.print(ch3_loc);  
	Serial.println();  Serial.println();

	msg = JSON.stringify(jsonReadings); // Coloca todas as leituras do local no msg
	mesh.sendBroadcast(msg); // Envia a mensagem para a rede MESH
}

//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------

void receivedCallback( uint32_t from, String &msg_recebida ) {	// recebe as mensagens da rede MESH
	#ifdef MSGS
	Serial.printf("Received from %u: %s\n", from, msg_recebida.c_str());
	#endif

	JSONVar myObject = JSON.parse(msg_recebida.c_str());  // coloca no myObject a msg_recebida

	node_rec = myObject["node"];  // extrai cada valor tendo no nome como referencia (precisa ser igual o que manda e recebe)
	ch1_rec = myObject["ch1"];
	ch2_rec = myObject["ch2"];
	ch3_rec = myObject["ch3"];

	Serial.print(" node_rec= "); Serial.print(node_rec);
	Serial.print("   ch1_rec= "); Serial.print(ch1_rec);
	Serial.print("   ch2_rec= "); Serial.print(ch2_rec);
	Serial.print("   ch3_rec= "); Serial.print(ch3_rec);

	Serial.println();
}

//---------------------------------------------------------------------------------------------------------------

void newConnectionCallback(uint32_t nodeId) {	// Callbacks de nova conexão para debug
	#ifdef DEBUG
	Serial.printf("New Connection, nodeId = %u\n", nodeId);
	#endif
}

//---------------------------------------------------------------------------------------------------------------

void changedConnectionCallback() {	// Callbacks de alteração de conexão para debug
	#ifdef DEBUG
	Serial.printf("Changed connections\n");
	#endif
}

//---------------------------------------------------------------------------------------------------------------

void nodeTimeAdjustedCallback(int32_t offset) {	// Callbacks de ajuste de tempo para debug
	#ifdef DEBUG
	Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
	#endif
}
//---------------------------------------------------------------------------------------------------------------

 float Leitura_Tensao_Canal(int canal, float n_amostras) {	// Leitura do canal analógico
	int i = 0;
	float media_leituras = 0;

	for (i = 0; i < n_amostras; i++) {
		media_leituras += analogRead(canal);
	}

	media_leituras = media_leituras / n_amostras * 3.3 / 4096.0;
	return media_leituras;
}
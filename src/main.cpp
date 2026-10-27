#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "BluetoothSerial.h"

// Credenciais do Wi-Fi
const char* ssid = "Redmi Note 14S";
const char* password = "56788269";

BluetoothSerial SerialBT;
WebServer server(8080);

// ============================================================================
// FUNÇÕES AUXILIARES
// ============================================================================

void enviarParaImpressora(const String& texto) {
  if (SerialBT.connected()) {
    SerialBT.print(texto);
  } else {
    Serial.println("Erro: Impressora nao conectada via Bluetooth!");
  }
}

void cortarPapel() {
  if (SerialBT.connected()) {
    // Avança 3 linhas e aciona a guilhotina (ESC/POS)
    SerialBT.write(0x1B); SerialBT.write(0x64); SerialBT.write(3);
    SerialBT.write(0x1D); SerialBT.write(0x56); SerialBT.write(65); SerialBT.write(0);
  }
}


// ============================================================================
// ROTA 1: COZINHA (/impressao/cozinha)
// ============================================================================

void handleCozinhaPedido() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"erro\": \"Body ausente\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"erro\": \"JSON invalido\"}");
    return;
  }

  String cupom;
  cupom.reserve(512); // Evita fragmentação de memória RAM

  int numeroPedido    = doc["numeroPedido"] | 0;
  int numeroMesa      = doc["numeroMesa"] | 0;
  const char* horario = doc["horario"] | "00:00";
  const char* garcom  = doc["nomeGarcom"] | "Sem nome";

  cupom += "================================\n";
  cupom += "       PEDIDO - COZINHA         \n";
  cupom += "================================\n";
  cupom += "Pedido #: " + String(numeroPedido) + "\n";
  cupom += "Mesa    : " + String(numeroMesa) + "\n";
  cupom += "Horario : " + String(horario) + "\n";
  cupom += "--------------------------------\n";

  JsonArray itens = doc["itens"].as<JsonArray>();
  for (JsonObject item : itens) {
    int qtd          = item["quantidade"] | 1;
    const char* nome = item["nomePrato"] | "";
    const char* obs  = item["observacoes"] | "";

    cupom += String(qtd) + "x " + String(nome) + "\n";
    if (strlen(obs) > 0) {
      cupom += " OBS: " + String(obs) + "\n";
    }
    cupom += "--------------------------------\n";
  }

  cupom += "Garcom  : " + String(garcom) + "\n";
  cupom += "================================\n";

  enviarParaImpressora(cupom);
  cortarPapel();

  server.send(200, "application/json", "{\"status\": \"Impresso na Cozinha\"}");
}

// ============================================================================
// ROTA 2: CAIXA (/impressao/caixa)
// ============================================================================

void handleCaixa() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"erro\": \"Body ausente\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"erro\": \"JSON invalido\"}");
    return;
  }

  String cupom;
  cupom.reserve(512);

  int numeroPedido        = doc["numeroPedido"] | 0;
  int numeroMesa          = doc["numeroMesa"] | 0;
  float subtotal          = doc["subtotal"] | 0.0;
  float taxaServico       = doc["taxaServico"] | 0.0;
  float valorDesconto     = doc["valorDesconto"] | 0.0;
  float valorTotal        = doc["valorTotal"] | 0.0;
  const char* nomeCliente = doc["nomeCliente"] | "";

  cupom += "================================\n";
  cupom += "     🦐 Restaurante SA 🦐      \n";
  cupom += "================================\n";
  cupom += "Pedido #: " + String(numeroPedido) + "\n";
  cupom += "Mesa    : " + String(numeroMesa) + "\n";
  cupom += "--------------------------------\n";

  JsonArray itens = doc["itens"].as<JsonArray>();
  for (JsonObject item : itens) {
    int qtd          = item["quantidade"] | 1;
    const char* nome = item["nomePrato"] | "";
    float valor      = item["valorPrato"] | 0.0;

    cupom += String(qtd) + "x " + String(nome) + "\n";
    cupom += "                      R$ " + String(valor, 2) + "\n";
    cupom += "--------------------------------\n";
  }

  cupom += "Subtotal      : R$ " + String(subtotal, 2) + "\n";
  cupom += "Taxa Servico  : R$ " + String(taxaServico, 2) + "\n";
  cupom += "Desconto      : R$ " + String(valorDesconto, 2) + "\n";
  cupom += "================================\n";
  cupom += "TOTAL         : R$ " + String(valorTotal, 2) + "\n";
  cupom += "================================\n";

  if (strlen(nomeCliente) > 0) {
    cupom += "Cliente       : " + String(nomeCliente) + "\n";
    cupom += "================================\n";
  }

  enviarParaImpressora(cupom);
  cortarPapel();

  server.send(200, "application/json", "{\"status\": \"Impresso no Caixa\"}");
}

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  
  // Inicializa o Bluetooth e conecta na impressora pelo nome exato
  SerialBT.begin("ESP_32", true);
  SerialBT.setPin("1234");
  Serial.println("Conectando a impressora Bluetooth...");
  SerialBT.connect("SMART PRINTER");

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nConectado! IP do ESP32: ");
  Serial.println(WiFi.localIP());

  // Mapeia as 2 rotas principais
  server.on("/impressao/cozinha", HTTP_POST, handleCozinhaPedido);
  server.on("/impressao/caixa", HTTP_POST, handleCaixa);

  server.begin();
}

void loop() {
  server.handleClient();
}
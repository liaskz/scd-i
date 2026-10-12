#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include "FS.h"
#include <LittleFS.h>
#include "Update.h"
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
//#include "HX711.h"

LiquidCrystal_I2C lcd(0x3f, 20, 4);
Preferences preferences;
//HX711 hx;

#define potPin 13
#define ButtonPin 4  // TODO: rename
#define PINO_CS 5
#define RXD2 16
#define TXD2 17
#define LOADCELL_DOUT_PIN 27
#define LOADCELL_SCK_PIN 14


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <style>
    header {
      background-color: #c1c1c1;
      color: black;
      height: 62px;
      text-align: center;
      margin-top: -20px;
      font-family: sans-serif;
      padding: 10px;
      border-bottom: 3px solid rgba(0, 231, 244, 0.8);
    }
  
    footer {
      background-color:#c1c1c1;
      color: #fff;
      padding: 10px;
      text-align: center;
      position: fixed;
      bottom: 0;
      width: 100%;
    }

    table {
      border-collapse: collapse;
      width: 90%;
      margin-left:5%;
      table-layout: fixed;

    }

   
    thead {
      background-color: rgba(0, 231, 244, 0.9);
    }

   
    .center{
      text-align: center;
      font-family: system-ui;

    }

    tbody tr:hover {
      background-color: rgba(0, 231, 244, 0.9);
    }

 

    .button-container button {
      font-size: 17px;
      margin-left: 5px;
      background-color: white;
      border: none;
      width: 47%;
      height: 45px;
      /* -webkit-transform: skew(-20deg); */
      -moz-transform: skew(-20deg);
      -o-transform: skew(-20deg);
      filter: drop-shadow(0 0 2px rgba(227, 234, 234, 0.6));
      stroke: rgba(184, 202, 203, 0.6);
      transition: background-color 0.1s;
    }
    .tbl-content{
      height:60vh;
      overflow-x:auto;
      margin-top: 0px;
     
      width: 90%;
      margin-left: 5%;
      filter: drop-shadow(0 0 2px rgba(0, 231, 244, 0.8));
      stroke: rgba(0, 231, 244, 0.9);
    }
    th{
      padding: 20px 15px;
      text-align: left;
      font-weight: 500;
      font-size: 20px;
     
      text-transform: uppercase;
    }
    td{
      background: rgba(49, 45, 45,0.3);
      padding: 23px;
      text-align: left;
      vertical-align:middle;
      color: #ffffff;
      font-size: 20px;
      
      border-bottom: solid 1px rgba(255,255,255,0.1);
    }
    body{
      margin: 0px;
      background:black;
    }
    .content_selct{
      width: 90%;
      margin-left: 5%;
      margin-bottom: 20px;
      justify-content: end;
      display: flex;
      border-radius: 0px;
    }
    .button_selct {
      width: 73px;
      display: flex;
      padding: 10px;
      border: none;
      justify-content: center;
      font-size: 20px;
      background: rgba(0, 231, 244, 0.4);
    }
    .button_selcted{
      background: rgba(0, 231, 244, 0.9);
    }
    ::-webkit-scrollbar {
      width: 6px;
    } 
    ::-webkit-scrollbar-track {
      -webkit-box-shadow: inset 0 0 6px rgb(255, 0, 85); 
    } 
    ::-webkit-scrollbar-thumb {
      -webkit-box-shadow: inset 0 0 6px rgb(255, 0, 85); 
    }

    

  </style>
</head>
<body>
  <header>
    <h1>PRD</h1>
  </header>
  <br>
  <div class="content_selct custom-select">
    <button id="b_sd" class="button_selct button_selcted ">SD</button>
    <button id="b_LittleFS" class="button_selct" disabled>LittleFS</button>

  </div>
  
  <div class="tbl-content">
    <table id="fileTable" style="width: 100%;margin-left:0px;" cellpadding="0" cellspacing="0" border="0">
      <thead>
        <tr>
          <th class="center">Name/Type (Clique para Download)</th>
          <th class="center" style="width: 100px">Size</th>
          <th class="center" style="width: 50px">Type</th>
          <th class="center" style="width: 150px;">Action</th>
        </tr>
      </thead>
      <tbody id="content_tabel">
    </table>
  </div>

  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;

    // Função executada quando a página carrega
    window.addEventListener('load', () => {
      initWebSocket();
      // Solicita a lista de arquivos do LittleFS ao carregar
      document.getElementById("b_sd").click(); 
    });

    function initWebSocket() {
      console.log('Trying to open a WebSocket connection...');
      websocket = new WebSocket(gateway);
      websocket.onopen = onOpen;
      websocket.onclose = onClose;
      websocket.onmessage = onMessage;
    }

    function onOpen(event) {
      console.log('Connection opened');
    }

    function onClose(event) {
      console.log('Connection closed');
      setTimeout(initWebSocket, 2000);
    }

    // Função que processa as mensagens (lista de arquivos) vindas do ESP32
    function onMessage(event) {
      // Converte o texto JSON em um objeto
      var file = JSON.parse(event.data);
      // Adiciona o arquivo na tabela da página
      addFileToTable(file.name, file.size, file.type, file.loc);
    }

    // Função para adicionar uma linha na tabela de arquivos
    function addFileToTable(name, size, type, loc) {
      var tbody = document.getElementById('content_tabel');
      var newRow = document.createElement('tr');
      newRow.id = name; // Atribui um ID à linha para poder deletá-la

      // Célula do Nome (agora é um link de download)
      var nameCell = newRow.insertCell(0);
      nameCell.classList.add("center");
      var link = document.createElement('a');
      link.href = `/download?file=${encodeURIComponent(name)}&loc=${loc}`;
      link.textContent = name;
      link.style.color = "white";
      nameCell.appendChild(link);

      // Célula do Tamanho
      var sizeCell = newRow.insertCell(1);
      sizeCell.textContent = size;
      sizeCell.classList.add("center");
      sizeCell.style.width = "100px";
      
      // Célula do Tipo
      var typeCell = newRow.insertCell(2);
      typeCell.textContent = type;
      typeCell.classList.add("center");
      typeCell.style.width = "50px";

      // Célula dos Botões de Ação
      var actionsCell = newRow.insertCell(3);
      actionsCell.classList.add("button-container", "center");
      actionsCell.style.width = "300px";
      
      var deleteButton = document.createElement('button');
      deleteButton.textContent = "Delete";
      // A função de deleção agora também recebe a localização do arquivo
      deleteButton.setAttribute("onclick", `deleteFile('${name}', '${loc}')`);
      actionsCell.appendChild(deleteButton);

      tbody.appendChild(newRow);
    }

    // Função para deletar um arquivo
    function deleteFile(name, loc) {
      var message = `Delete/${loc}:${name}`;
      console.log("Enviando comando de deleção:", message);
      websocket.send(message);
      document.getElementById(name).remove();
    }
    
    // Limpa todas as linhas da tabela
    function clearTable() {
      document.getElementById('content_tabel').innerHTML = '';
    }

    // Evento de clique para o botão SD
    document.getElementById("b_sd").addEventListener("click", () => {
      document.getElementById("b_sd").classList.add("button_selcted");
      document.getElementById("b_LittleFS").classList.remove("button_selcted");
      clearTable();
      websocket.send("SD");
    });

    // Evento de clique para o botão LittleFS
    document.getElementById("b_LittleFS").addEventListener("click", () => {
      document.getElementById("b_LittleFS").classList.add("button_selcted");
      document.getElementById("b_sd").classList.remove("button_selcted");
      clearTable();
      websocket.send("LittleFS");
    });
    
  </script>
</body>
</html>
)rawliteral";


// Handle para a tarefa do Core 0 e para o Mutex do SD Card
TaskHandle_t webServerTaskHandle = NULL;
SemaphoreHandle_t sdMutex;

// Variáveis para o Web Server
const char* ssid = "SCD";
const char* password = "123456789";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


struct Menu {
  const char* name;
  const char* options[2];
};

Menu mainMenu[] = {
  { "Inicializar", { "OFF", "ON" } },
  { "Armazenamento", { "SD", "LittleFS" } },
  { "Transmissao", { "Desl", "Lig" } },
};



int selectedMenu = 0;
int activedMenu = 0;
int selected = 0;
bool coleta = false, sd = true, trans = false, firstOpen =  true;
long timeInit = 0, fz_lcd = 0;
unsigned long emp;
String pathSave;
int long convert, prev_convert, soma = 0, media = 0;
unsigned long col_time = 0, t2 = 0, timeToClose = 2000;
int lastButtonState = LOW;

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    char* msg = (char*)data; // Cast para char* uma vez para facilitar a leitura

    // --- Lógica de Deleção Robusta ---
    // Usamos strcasestr para uma busca que ignora maiúsculas/minúsculas
    
    // Procura por "Delete/SD:" independentemente do caso
    const char* sd_key = "Delete/SD:";
    char* substring = strcasestr(msg, sd_key);
    if (substring != NULL) {
      char* content = substring + strlen(sd_key);
      Serial.println("Comando para deletar arquivo SD recebido.");
      delete_file(true, content);
      return; // Sai da função após tratar o comando
    }

    // Procura por "Delete/LittleFS:" independentemente do caso, apesar que não vamos usar esse modo :p
    const char* lfs_key = "Delete/LittleFS:";
    substring = strcasestr(msg, lfs_key);
    if (substring != NULL) {
      char* content = substring + strlen(lfs_key);
      Serial.println("Comando para deletar arquivo LittleFS recebido.");
      delete_file(false, content);
      return; // Sai da função após tratar o comando
    }
    if (strcmp(msg, "SD") == 0) {
      dir(true);
    }
    if (strcmp(msg, "LittleFS") == 0) {
      dir(false);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}
void dir(bool op) {
  // A listagem de LittleFS está desativada para evitar o bug da placa
  if (op == false) {
    ws.textAll("{\"name\":\"Listagem desativada.\",\"size\":\"\",\"type\":\"Info\",\"loc\":\"LittleFS\"}");
    return;
  }

  File root;
  // Pega o mutex apenas para abrir o diretório raiz
  if (xSemaphoreTake(sdMutex, (TickType_t)10) == pdTRUE) {
    root = SD.open("/");
    xSemaphoreGive(sdMutex); // Devolve a chave imediatamente
  } else {
    ws.textAll("{\"name\":\"Falha ao acessar SD\",\"size\":\"\",\"type\":\"Error\",\"loc\":\"sd\"}");
    return;
  }

  if (!root || !root.isDirectory()) {
    return;
  }

  File file;
  // Loop para pegar o próximo arquivo
  while (true) {
    // Pega o mutex SÓ para pegar o próximo arquivo
    if (xSemaphoreTake(sdMutex, (TickType_t)10) == pdTRUE) {
      file = root.openNextFile();
      xSemaphoreGive(sdMutex); // Devolve a chave
    } else {
      break; // Sai do loop se não conseguir pegar o mutex
    }

    if (!file) {
      break; // Não há mais arquivos, sai do loop
    }
    
    // Pega o mutex novamente só para ler as propriedades e enviar
    if (xSemaphoreTake(sdMutex, (TickType_t)10) == pdTRUE) {
      String fsize = "";
      int bytes = file.size();
      if (bytes < 1024) fsize = String(bytes) + " B";
      else if (bytes < (1024 * 1024)) fsize = String(bytes / 1024.0, 3) + " KB";
      else fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";

      String location = op ? "sd" : "LittleFS";
      ws.textAll("{\"name\":\"" + String(file.name()) + "\",\"size\":\"" + fsize + "\",\"type\":\"" + (file.isDirectory() ? "Dir" : "File") + "\",\"loc\":\"" + location + "\"}");
      
      file.close(); // Fecha o handle do arquivo atual
      xSemaphoreGive(sdMutex); // Devolve a chave
    }
    delay(10);
  }
  // Fecha o diretório raiz no final
  if (xSemaphoreTake(sdMutex, (TickType_t)10) == pdTRUE) {
    root.close();
    xSemaphoreGive(sdMutex);
  }
}
void delete_file(bool op, String filename) {
  if (xSemaphoreTake(sdMutex, (TickType_t)10) == pdTRUE) {
    if (op) { // op == true significa SD Card
      Serial.println("Deletando do SD: /" + filename);
      SD.remove("/" + filename); 
    } else { // op == false significa LittleFS
      Serial.println("Deletando do LittleFS: /" + filename);
      LittleFS.remove("/" + filename); 
    }
    xSemaphoreGive(sdMutex);
  } else {
    Serial.println("Nao foi possivel obter mutex para deletar arquivo.");
  }
}

// Esta função será executada no Core 0
void webServerTask(void *pvParameters) {
  Serial.println("Tarefa do Web Server iniciada no Core 0.");

  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  ws.onEvent(onEvent);
  
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (xSemaphoreTake(sdMutex, (TickType_t)10) == pdTRUE) {
      request->send_P(200, "text/html", index_html);
      xSemaphoreGive(sdMutex);
    } else {
      request->send(503, "text/plain", "SD Card ocupado, tente novamente mais tarde :p.");
    }
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    
    if (xSemaphoreTake(sdMutex, (TickType_t)10) == pdTRUE) {
  
      // Verificação se os parâmetros 'file' e 'loc' foram enviados na URL
      if (request->hasParam("file") && request->hasParam("loc")) {
        String fileName = request->getParam("file")->value();
        String location = request->getParam("loc")->value();
        String path = "/" + fileName;
  
        FS* filesystem;
  
        if (location == "sd") {
          filesystem = &SD;
        } else if (location == "LittleFS") {
          filesystem = &LittleFS;
        } else {
          request->send(400, "text/plain", "Localizacao invalida.");
          xSemaphoreGive(sdMutex);
          return;
        }
        
        // Verificação se o arquivo realmente existe no local especificado
        if (filesystem->exists(path)) {
          Serial.println("Enviando arquivo para download: " + path + " com o nome: " + fileName);
          AsyncWebServerResponse *response = request->beginResponse(*filesystem, path, "application/octet-stream");
          response->addHeader("Content-Disposition", "attachment; filename=" + fileName);
          request->send(response);

        } else {
          Serial.println("Arquivo nao encontrado: " + path);
          request->send(404, "text/plain", "Arquivo nao encontrado.");
        }
        
      } else {
        request->send(400, "text/plain", "Parametros 'file' e 'loc' faltando.");
      }
      
      xSemaphoreGive(sdMutex);
  
    } else {
      request->send(503, "text/plain", "Recurso ocupado, tente novamente.");
    }
  });

  server.begin();
  for (;;) {
    ws.cleanupClients();
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}

/**
 * Escrita de menus no LCD.
 *
 * @var activedMenu: estado atual do Menu:
 *    - 0: Seleção de opções de Menu;
 *    - 1: Inicializar
 *    - 2: Armazenamento
 *    - 3: Transmissao
 */
void printMenu() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("SCD - PRD");

  if (activedMenu == 0) {
    // lista opções de menu
    for (int i = 1; i < 4; i++) {
      lcd.setCursor(1, i);
      lcd.print(mainMenu[i - 1].name);
    }
  } else {
    // printa menu selecionado
    lcd.setCursor(1, 1);
    lcd.print(mainMenu[activedMenu - 1].options[0]);
    lcd.setCursor(1, 2);
    lcd.print(mainMenu[activedMenu - 1].options[1]);
    if (activedMenu == 1) {
      lcd.setCursor(0, 3);
      lcd.print(" Empuxo: ");
    }
  }
}

/**
* Seleção de menus no LCD lendo o potenciômetro.
*/
void menuSelect() {
  int new_data = analogRead(potPin);
  int prev_selectedMenu = selectedMenu;

  if (activedMenu == 0) {
    //new_data = map(analogRead(potPin),4095,0,0,2); Modelado por log, linear é melhor.
    if (new_data > 2730) selectedMenu = 3;   // TODO: adicionar buffer entre intervalos
    else if (new_data > 1365) selectedMenu = 2;
    else selectedMenu = 1;
  } else {
    if (new_data > 2043) selectedMenu = 2;
    else selectedMenu = 1;
  }
  if (prev_selectedMenu != selectedMenu) {  // FIXME: duas escritas de cursor
    lcd.setCursor(0, prev_selectedMenu);
    lcd.print(" ");
    lcd.setCursor(0, selectedMenu);
    lcd.print(">");
  }
}

void startCollection() {
  // Apenas inicia a coleta se ela já não estiver ativa
  if (!coleta) {
    coleta = true;
    Serial.println("Coleta INICIADA."); //TODO: adicionar o indicativo de coleta iniciada na interface do lcd
    
    // Define o nome do arquivo para a nova sessão de coleta
    preferences.putInt("cont", preferences.getInt("cont", 0) + 1);
    pathSave = "/Coleta_" + String(preferences.getInt("cont", 0)) + ".txt";
    Serial.println("Salvando em: " + pathSave);
    
    // Atualiza o LCD para o "Modo Coleta"
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("COLETA ATIVA...");
    lcd.setCursor(0, 2);
    lcd.print(pathSave);
    lcd.setCursor(0, 3);
    lcd.print("Pressione para Parar");
  }
}
void stopCollection() {
  // Apenas para a coleta se ela estiver ativa
  if (coleta) {
    coleta = false;
    Serial.println("Coleta PARADA.");
    
    // Redesenha o menu principal para sinalizar o fim do modo de coleta
    printMenu();
  }
}

void handleCollectionMode() {
  coletaEmp(); 
  int currentButtonState = digitalRead(ButtonPin);

  // Verifica se houve uma MUDANÇA de estado: de NÃO pressionado (LOW) para PRESSIONADO (HIGH).
  // Isso detecta a "borda de subida" do clique
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    //Serial.println("Clique detectado, saindo do modo de coleta...");
    stopCollection(); // Chama a função para parar tudo de forma organizada.
  }
  lastButtonState = currentButtonState;
}

// Função para iniciar o servidor
void startWebServer() {
  // Se a tarefa não estiver rodando (handle é NULL), então ela é criada :D
  if (webServerTaskHandle == NULL) {
    Serial.println("Ligando a transmissao... Criando tarefa no Core 0.");

    // Atualiza o LCD para o "Modo Transmissão"
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Rede Wi-Fi Ativa");
    lcd.setCursor(0, 1);
    lcd.print("IP: 192.168.4.1");
    lcd.setCursor(0, 3);
    lcd.print("Pressione para Sair");
    
    //delay(2000);
    xTaskCreatePinnedToCore(
      webServerTask,      // Função da tarefa
      "WebServerTask",    // Nome
      5000,              // Tamanho da Pilha (Stack Size)
      NULL,               // Parâmetros
      1,                  // Prioridade
      &webServerTaskHandle, // Handle da Tarefa
      0);                 // Fixar no Core 0
  } else {
    Serial.println("Transmissao ja esta ligada.");
  }
}

// Função para parar o servidor
void stopWebServer() {
  if (webServerTaskHandle != NULL) {
    Serial.println("Desligando a transmissao... Deletando tarefa do Core 0.");
    
    ws.closeAll();
    server.end();
    
    vTaskDelete(webServerTaskHandle);
    webServerTaskHandle = NULL;
    
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi e Servidor desligados.");
  } else {
    Serial.println("Transmissao ja esta desligada.");
  }
}

void handleTransmissionMode() {
  int currentButtonState = digitalRead(ButtonPin);
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    Serial.println("Botao pressionado, saindo do modo de transmissao.");
    trans = false;
    stopWebServer();
    activedMenu = 0;
    selectedMenu = 0;
    printMenu();
  }
  lastButtonState = currentButtonState;
}

/**
 * Referente às transições e seleção de menu ao clicar o botão
 */
void click() {
  int currentButtonState = digitalRead(ButtonPin);
  //TODO: arrumar o debounce do botão
  if (lastButtonState == LOW && currentButtonState == HIGH) { // Ação apenas na transição
    if (activedMenu != 0) { // Se estamos em um submenu...
      
      if (activedMenu == 1) { // Menu "Inicializar"
        if (selectedMenu == 2) startCollection(); // Se selecionou "ON"
        // Não precisamos de 'else' para parar, pois isso é feito em handleCollectionMode
        activedMenu = 0;
        selectedMenu = 0;
      } 
      else if (activedMenu == 2) { // Menu "Armazenamento"
        sd = (selectedMenu == 1);
        activedMenu = 0;
        selectedMenu = 0;
      } 
      else if (activedMenu == 3) { // Menu "Transmissao"
        if (selectedMenu == 2) { // Se selecionou "Lig"
          trans = true;
          startWebServer();
        } else { // Se selecionou "Desl"
          trans = false;
          stopWebServer();
          activedMenu = 0;
          selectedMenu = 0;
        }
      }
    } else { // Se estamos no menu principal...
      activedMenu = selectedMenu; // Entra no submenu
    }
  }
  lastButtonState = currentButtonState;
}

void save() {
  if (xSemaphoreTake(sdMutex, (TickType_t)10) == pdTRUE) {
    File fileToSave = SD.open(pathSave, FILE_APPEND);
    
    // Verifica se o arquivo foi aberto com sucesso
    if (fileToSave) {
      char dataBuffer[30]; 
      sprintf(dataBuffer, "%lu %lu", millis(), emp);
      fileToSave.println(dataBuffer);
      fileToSave.close(); 
    } else {
      Serial.println("Falha ao abrir arquivo para salvar!");
    }
    xSemaphoreGive(sdMutex); 

  } else {
    Serial.println("Falha ao obter mutex do SD para salvar dados.");
  }
}

unsigned long ReadCount() {

  unsigned long Count = 0;
  unsigned char i;
  if (!digitalRead(LOADCELL_DOUT_PIN)) {
    col_time = millis();
    digitalWrite(LOADCELL_SCK_PIN, LOW);
    //digitalRead(LOADCELL_DOUT_PIN);

    for (i = 0; i < 24; i++) {
      digitalWrite(LOADCELL_SCK_PIN, HIGH);
      Count = Count << 1;
      digitalWrite(LOADCELL_SCK_PIN, LOW);
      if (digitalRead(LOADCELL_DOUT_PIN)) Count++;

    }  //end for

    digitalWrite(LOADCELL_SCK_PIN, HIGH);
    Count = Count ^ 0x800000;
    digitalWrite(LOADCELL_SCK_PIN, LOW);
    return (Count);
  }
  return 0;
}

/**
 * Coleta dos dados
 */
void coletaEmp() {
  emp = ReadCount();
  unsigned long t1;
  if (coleta == true) {
    //Serial.println(emp);
    if (emp != 0) {
      save();
    }
  } else {
    if (activedMenu == 1) {
      if (millis() - fz_lcd > 500) {
        fz_lcd = millis();
        lcd.setCursor(9, 3);
        lcd.print("        ");
        lcd.setCursor(9, 3);
        lcd.print(emp);
      }
    }
  }
}

void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(115200);
  
  pinMode(ButtonPin, INPUT_PULLDOWN);
  pinMode(potPin, INPUT);
  pinMode(LOADCELL_DOUT_PIN, INPUT_PULLUP);  //entrada para receber os dados
  pinMode(LOADCELL_SCK_PIN, OUTPUT);

  // Cria o Mutex que irá proteger o acesso ao SD Card e LittleFS
  sdMutex = xSemaphoreCreateMutex();
  
  //hx.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  if (!SD.begin()) {
    Serial.println("Falha ao inicializar o cartão SD.");  // TODO: mostrar erro no display
  }
//  if (!LittleFS.begin(true)) {
//    Serial.println("Falha ao inicializar o LittleFS.");
//  }
  preferences.begin("storage", false);
  preferences.putInt("cont", preferences.getInt("cont", 0) + 1);
  pathSave = "/Coleta_" + String(preferences.getInt("cont", 0)) + ".txt";
  Serial.println("SCD inicializado com sucesso.");
  printMenu();
}

void loop() {
  // Guarda o estado do menu no início de cada ciclo para detectar mudanças
  int prev_activedMenu = activedMenu;
  if (trans == true) {
    // ESTADO 1: Transmissão Ativa
    // Apenas a lógica de sair da transmissão é necessária.
    handleTransmissionMode();
    delay(10);
  } 
  else if (coleta == true) { 
    // ESTADO 2: Coleta Ativa
    // A lógica de coletar e esperar o botão de parada está toda aqui.
    handleCollectionMode();
  } 
  else {
    // ESTADO 3: Menu Principal
    // A navegação e seleção do menu estão ativas.
    menuSelect();
    click();
    // Lógica para mostrar o valor do empuxo AO VIVO, mas SÓ na tela do submenu "Inicializar"
    if (activedMenu == 1) {
      coletaEmp(); 
    }
  }
  if (activedMenu != prev_activedMenu && !trans && !coleta) {
    printMenu(); // Redesenha a tela do menu/submenu
    // E redesenha o cursor na posição correta
    for (int i = 1; i < 4; i++) {
      lcd.setCursor(0, i);
      lcd.print(" ");
    }
    lcd.setCursor(0, selectedMenu);
    lcd.print(">");
    delay(10);
  }
}

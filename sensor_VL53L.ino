// Biblioteca que lida com o sensor VL53L0X
#include <VL53L0X.h>
// Biblioteca que lida com a interface I²C
#include <Wire.h>

// Definição dos pinos dos botões principal e de configuração
#define MAIN_BUTTON A0
#define CONFIG_BUTTON A1

// Intervalo de tempo entre as fases em milisegundos - min 2 para Wave Step 
int phase_delay = 2;

// Usar isso pra saber a distância do sensor até o fundo da plataforma e saber, assim, se a medida deve ou não ser descartada, por exemplo.
float back_distance = 0;

// Quantos passos a base deu
int baseSteps = 0;

// Quantos passos o sensor deu
int sensorSteps = 0;

// Última distância medida pelo sensor
float lastDistance = 0;

// Instância global do Sensor VL53L0X
VL53L0X sensor;

// Controla se o sensor está configurado na altura inicial certa
bool isConfigured = false;

class Point {
  public:
    float x = 0;
    float y = 0;
    float z = 0;

    Point(float X, float Y, float Z) {
      x = X;
      y = Y;
      z = Z;
    }
    
    void toSerial() {
      Serial.print(x);
      Serial.print(",");
      Serial.print(y);
      Serial.print(",");
      Serial.print(z);
      Serial.println("");
    }
};

// Retorna um Point contendo os 3 pontos (x,y,z) encapsulados
Point measure3DPoint() {

  lastDistance = sensor.readRangeContinuousMillimeters();
 
  float inRadians = baseSteps * 0.7 * 3.14159265 / 180;
  float x = cos(inRadians) * (75 - lastDistance);
  float y = -1 * sin(inRadians) * (75 - lastDistance);
  // Sabendo que 9 é o número de voltas que equivalem a um centímetro de deslocamento e 512 é o número de passos necessários para uma volta
  float z = sensorSteps / 512;
  
  Point point(x, y, z);

  return point;
}

// Realiza um passo completo (FullStep) no motor do sensor no sentido horário ou anti-horário
void FullStepSensor(bool clockwise = true) {
  // Matriz dos bytes das fases do motor da base. PORTB - portas digitais 8 a 13
  byte matrix[4] = {B00011000, B00110000, B00100100, B00001100}; // TO-DO: hexadecimal

  for(int j = 0; j < 4; j++) {
    delay (phase_delay); // Atraso de tempo entre as fases em milisegundos
    if(clockwise) {
      PORTD = matrix[j];
    }
    else {
      PORTD = matrix[3 - j];
    }
  }
  PORTD = 0x00;
  sensorSteps += 1;
}

// Realiza um passo completo (FullStep) no motor da base no sentido horário ou anti-horário
void FullStepBase(bool clockwise = true) {
  // Matriz dos bytes das fases do motor da base. PORTD - portas digitais 0 a 7
  byte matrix[4] = {0x09,0x03,0x06,0x0C};    
  
  for(int j = 0; j < 4; j++) {
    delay (phase_delay); // Atraso de tempo entre as fases em milisegundos
    if(clockwise) {
      PORTB = matrix[j];
    } else {
      PORTB = matrix[3 - j];
    }
  }
  PORTB = 0x00;
  baseSteps += 1;
}

// Realiza uma rotação completa (360°) do motor da base
void FullRotationBase(bool cw) {
  for(int i = 0; i < 512; i++) {
    FullStepBase(cw);
  }
}

// Realiza uma rotação completa (360°) do motor do sensor
void FullRotationSensor(bool cw = true) {
  for(int i = 0; i < 512; i++) {
    FullStepSensor(cw);
  }
}

// Move o sensor a quantidade de centímetros específicados em distance_in_cm, que tem padrão 1, para cima ou para baixo de acordo com a variável up, a qual tem padrão true
void moveSensorInCM(int distance_in_cm = 1, bool up = true) {
  int voltas = distance_in_cm * 9; // 9 sendo o número de voltas necessárias para subir 1cm (previamente calculado)

  for (int i = 0; i < voltas; i++) {
    FullRotationSensor(up); // TO-DO: tem que ver se não tá trocado 0 e 1
  }
}

// Move o sensor para o ponto inicial e reinicia todas variáveis necessárias para o scan.
void resetScan() {
  baseSteps = 0;

  // Leva o sensor para o ponto inicial (tudo que ele tinha subido, desce)
  for (int steps = 0; steps < sensorSteps; steps++) {
    FullStepSensor(false);
  }
  sensorSteps = 0;
}

// Escanear o objeto
void scan () {
  Serial.println("comecou escaneamento");
  delay(500);
  baseSteps = 0;
  sensorSteps = 0;
  while (true) {
    // Se uma volta da base tiver sido completada (360°)...
    if (baseSteps == 512) {
      
      for (int x = 0; i < 3; i++) {
        FullRotationSensor();
      }

      // Zera baseSteps
      baseSteps = 0;
    }

    // Faz uma medição
    Point measure = measure3DPoint();

    // Se a medida de profundidade for igual à distância para a parede traseira, significa que a medição acabou.
    // Se a medição acabar ou o botão principal for presisonado...
    if (not digitalRead(MAIN_BUTTON)) {
      // Finaliza o processo de medição
      Serial.println("done");
      delay(500);
      break;
    }
    if (lastDistance >= back_distance) {
      Serial.println("Possível fim");
      measure = Point(0, 0, 0);
    }
    measure.toSerial();

    // Da um passo no motor da base
    FullStepBase();
    delay(50);
  };
}

// Configurar a altura do sensor antes de cpmeçar a medição e a distância da parede do fundo
void configureSensor() {

  bool canGoUp = false;
  bool canGoDown = false;
  
  while(true) {
    if (not digitalRead(MAIN_BUTTON) and (not digitalRead(CONFIG_BUTTON))) {
      Serial.println("Configurado");
      delay(500); // Esperar meio segundo para o click não ser confundido com o de start
      break;
    } 
    while (not digitalRead(MAIN_BUTTON) and (digitalRead(CONFIG_BUTTON))) {
      FullStepSensor();
      Serial.println("Subindo");
    } 
    while (digitalRead(MAIN_BUTTON) and (not digitalRead(CONFIG_BUTTON))) {
      FullStepSensor(false);
      Serial.println("Descendo");
    }
    Point measure = measure3DPoint();
    Serial.println(lastDistance); // Assim é possível ter uma noção mais precisa
  }
  back_distance = lastDistance; // Define a distância até a parede do fundo
  isConfigured = true;
}

void setup() {
  // Inicia a interface I²C
  Wire.begin();
  // Inicia o Serial
  Serial.begin(9600);
  
  // Definição dos botões principal e de configuração
  pinMode(MAIN_BUTTON, INPUT_PULLUP);
  pinMode(CONFIG_BUTTON, INPUT_PULLUP);

  // TO-DO: fazer isso de maneira eficiente
  // Definições de pinos diretamente
  DDRB = 0x0F;   // Configura Portas D08,D09,D10 e D11 como saída 
  DDRD = 0xFF;   // Configura Portas D00 à D07 como saída
  PORTB = 0x00;  // Set dos estados da Porta B (D08 a D15) para LOW
  PORTD = 0x00;  // Set dos estados da Porta D (D00 a D07) para LOW

  // Configurando Sensor VL53L0X
  sensor.init();
  sensor.setTimeout(500);
  sensor.setMeasurementTimingBudget(50000); //TO-DO: mudar no artigo, tava 200000
  sensor.startContinuous();
}

void loop() {
  // Se o sensor ainda não foi configurado...
  if (not isConfigured) {
    // Configurar
    configureSensor();
  }

  // Quando o botão for pressionado...
  if (not digitalRead(MAIN_BUTTON)) {
    // Escanear o objeto
    scan();
    // Reiniciar as configurações e altura do sensor 
    resetScan();
  }
}

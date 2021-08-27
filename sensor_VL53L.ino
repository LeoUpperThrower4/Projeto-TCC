#include <VL53L0X.h>
#include <Wire.h>

// Botão principal
#define MAIN_BUTTON A0

// Intervalo de tempo entre as fases em milisegundos - min 2 para Wave Step 
int phase_delay = 2 ;

// Usar isso pra saber a distância do sensor até o fundo da plataforma e saber, assim, se a medida deve ou não ser descartada, por exemplo.
#define BACK_DISTANCE 7000 // 800 (distância em mm) - 100 (margem em mm)

// Quantos passos a base deu
int baseSteps = 0;

// Criando instância global do Sensor VL53L0X
VL53L0X sensor;

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
    
    void toSerial()
    {
      Serial.print(x);
      Serial.print(",");
      Serial.print(y);
      Serial.print(",");
      Serial.print(z);
      Serial.println("");
    }
};

// Retorna um float contendo a medição do ponto 'x'
float getX() { 
    return 1; 
}

// Retorna um float contendo a medição do ponto 'y'
float getY() { 
  return 1;
}

// Retorna um float contendo a medição do ponto 'z'
float getZ() {
  return sensor.readRangeContinuousMillimeters();
}

// Retorna um Point contendo os 3 pontos (x,y,z) encapsulados
Point measure3DPoint() {
  Point point(getX(), getY(), getZ());

  return point;
}

// Realiza um passo FullStep no motor do sensor
void FullStepSensor(int cw) {
  // Matriz dos bytes das fases do motor da base. PORTB - portas digitais 8 à 13
  byte HOR[4] = {B00011000, B00110000, B00100100, B00001100};    // Sentido Horário FullStep
  byte AHO[4] = {B00001100, B00100100, B00110000, B00011000};    // Sentido Anti-Horário FullStep

  for(int j = 0; j < 4; j++)      // incrementa o contador j de 0 a 3 
    {
      if(cw) {
        PORTD = HOR[j];                // Carrega bytes da Matriz HOR na Porta D 
      }
      else {
        PORTD = AHO[j];                // Carrega bytes da Matriz AHO na Porta D 
      }
      delay (phase_delay);          // Atraso de tempo entre as fases em milisegundos
    }
}

// Realiza um passo FullStep no motor da base
void FullStepBase(int cw) {
  // Matriz dos bytes das fases do motor do sensor (PORTD) portas digitais: 0 à 7
  byte HOR[4] = {0x09,0x03,0x06,0x0C};    
  // Matriz dos bytes das Fases do Motor - sentido Horário Full Step (maior torque)
  byte AHO[4] = {0x0C,0x06,0x03,0x09};
  
  for(int j = 0; j < 4; j++)      // incrementa o contador j de 0 a 3 
  {
    if(cw) {
      PORTB = HOR[j];               // Carrega bytes da Matriz AHO na Porta B 
    } else {
      PORTB = AHO[j];
    }
    delay (phase_delay);          // Atraso de tempo entre as fases em milisegundos
  }
}

// Realiza uma rotação completa (360°) do motor da base
void FullRotationBase(int cw) {
  for(int i = 0; i < 512; i++)      // incrementa o contador i de 0 a 511 - uma volta
  {
    FullStepBase(cw);
  }
}

// Realiza uma rotação completa (360°) do motor do sensor
void FullRotationSensor(int cw) {
  for(int i = 0; i < 512; i++)
  {
    FullStepSensor(cw);
  }
}

void setup() {
  Wire.begin();       // Inicia a interface I²C
  Serial.begin(9600); // Inicia o Serial
  
  // Definição botão start/stop
  pinMode(MAIN_BUTTON, INPUT_PULLUP);

  // TO-DO: fazer isso de maneira eficiente
  // Definições de pinos diretamente
  DDRB = 0x0F;   // Configura Portas D08,D09,D10 e D11 como saída 
  DDRD = 0xFF;   // Configura Portas D00 à D07 como saída
  PORTB = 0x00;  // Set dos estados da Porta B (D08 a D15) para LOW
  PORTD = 0x00;  // Set dos estados da Porta D (D00 a D07) para LOW

  // Configurando Sensor VL53L0X
  sensor.init();
  sensor.setTimeout(500);
  sensor.setMeasurementTimingBudget(200000);
  sensor.startContinuous();

}

// Um passo no motor da base
void rotateBaseOneStep() {
  FullStepBase(1);
  baseSteps += 1;
}

// Move o sensor a quantidade de centímetros específicados em distance_in_cm, que tem padrão 1, para cima ou para baixo de acordo com a variável up, a qual tem padrão true
void moveSensorInCM(int distance_in_cm = 1, bool up = true) {
  int voltas = distance_in_cm * 9; // 9 sendo o número de voltas necessárias para subir 1cm (previamente calculado)

  for (int i = 0; i < voltas; i++) {
    if (up) {
      FullRotationSensor(1); // TO-DO: tem que ver se não tá trocado 0 e 1
    }
    else {
      FullRotationSensor(0);
    }
  }
}

// Move o sensor para o ponto inicial e reinicia todas variáveis necessárias para o scan.
void resetScan() {
  baseSteps = 0;
  
}

void scan () {
  resetScan();
  while (true) {
    // Sobe o sensor 1cm
    moveSensorInCM();

    // Se uma volta da base tiver sido completada (360°)...
    if (baseSteps == 36) {
      // Parar a medição
      break;
    }
    // Se o botão principal for presisonado...
    if (not digitalRead(MAIN_BUTTON)) {
      // Parar a medição
      break;
    }
  };
}


void loop() {
  // Quando o botão for pressionado...
  if (not digitalRead(MAIN_BUTTON)) {
    Serial.println("iniciou");
    moveSensorInCM();
    Point ponto = measure3DPoint();
    Serial.println(ponto.z);
    Serial.println("done");
  }
}

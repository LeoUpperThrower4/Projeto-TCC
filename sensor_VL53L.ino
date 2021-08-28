// Biblioteca que lida com o sensor VL53L0X
#include <VL53L0X.h>
// Biblioteca que lida com a interface I²C
#include <Wire.h>

// Definição dos pinos dos botões principal e de configuração
#define MAIN_BUTTON A0
#define CONFIG_BUTTON A1

// Intervalo de tempo entre as fases em milisegundos - min 2 para Wave Step 
int phase_delay = 2 ;

// Usar isso pra saber a distância do sensor até o fundo da plataforma e saber, assim, se a medida deve ou não ser descartada, por exemplo.
int back_distance = 0;

// Quantos passos a base deu
int baseSteps = 0;

// Quantos passos o sensor deu
int sensorSteps = 0;

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
// cw -> 0 - Anticlockwise; qualquer outro - Clockwise // TO-DO: mudar para bool ao inves de int
void FullStepSensor(int cw) {
  // Matriz dos bytes das fases do motor da base. PORTB - portas digitais 8 a 13
  byte matrix[4] = {B00011000, B00110000, B00100100, B00001100}; // TO-DO: hexadecimal

  for(int j = 0; j < 4; j++) {
    if(cw) {
      PORTD = matrix[j];
      // TO-DO: saber isso -> Se no sentido horário sobe
      sensorSteps += 1; // TO-DO: pode ser sensorSteps = sensorSteps + 1;
    }
    else {
      PORTD = matrix[3 - j];
      sensorSteps -= 1; // TO-DO: pode ser sensorSteps = sensorSteps - 1;
    }
    delay (phase_delay); // Atraso de tempo entre as fases em milisegundos
  }
}

// Realiza um passo FullStep (maior torque) no motor da base
void FullStepBase(int cw = 1) {
  // Matriz dos bytes das fases do motor da base. PORTD - portas digitais 0 a 7
  byte matrix[4] = {0x09,0x03,0x06,0x0C};    
  
  for(int j = 0; j < 4; j++)
  {
    if(cw) {
      PORTB = matrix[j];
    } else {
      PORTB = matrix[3 - j];
    }
    delay(phase_delay); // Atraso de tempo entre as fases em milisegundos
    baseSteps += 1; // TO-DO: pode ser baseSteps = baseSteps + 1;
  }
}

// Realiza uma rotação completa (360°) do motor da base
void FullRotationBase(int cw) {
  for(int i = 0; i < 512; i++) {
    FullStepBase(cw);
  }
}

// Realiza uma rotação completa (360°) do motor do sensor
void FullRotationSensor(int cw) {
  for(int i = 0; i < 512; i++) {
    FullStepSensor(cw);
  }
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

  // Leva o sensor para o ponto inicial (tudo que ele tinha subido, desce)
  for (int steps = 0; steps < sensorSteps; steps++) {
    FullStepSensor(0);
  }
  sensorSteps = 0;
}

// Escanear o objeto
void scan () {
  while (true) {
    // Se uma volta da base tiver sido completada (360°)...
    if (baseSteps == 512) { // TO-DO: saber se é 512 ou 511 para uma volta completa
      // Da dois passos no motor do sensor
      for (int reps = 0; i < 2; i++) {
        FullStepSensor();
      }

      // Zera baseSteps
      baseSteps = 0;
    }

    // Faz uma medição
    Point measure = measure3DPoint();

    // Se a medida de profundidade for igual à distância para a parede traseira, significa que a medição acabou.
    // Se a medição acabar ou o botão principal for presisonado...
    if (measure.z == back_distance || not digitalRead(MAIN_BUTTON)) {
      // Finaliza o processo de medição
      Serial.println("done");
      break;
    }
    measure.toSerial();

    // Da um passo no motor da base
    FullStepBase();
  };
}

// Configurar a altura do sensor antes de cpmeçar a medição e a distância da parede do fundo
void configureSensor() {
  while(true) {
    if (not digitalRead(MAIN_BUTTON) and (not digitalRead(CONFIG_BUTTON))) {
      Serial.println("Configurado");
      delay(500); // Esperar meio segundo para o click não ser confundido com o de start
      break;
    } else if (not digitalRead(MAIN_BUTTON) and (digitalRead(CONFIG_BUTTON))) {
      FullStepSensor(1);
      Serial.println("Subindo");
    } else if (digitalRead(MAIN_BUTTON) and (not digitalRead(CONFIG_BUTTON))) {
      FullStepSensor(0);
      Serial.println("Descendo");
    }
    Serial.println(getZ()) // Assim é possível ter uma noção mais precisa
  }

  back_distance = getZ() // Define a distância até a parede do fundo

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
  sensor.setMeasurementTimingBudget(200000);
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

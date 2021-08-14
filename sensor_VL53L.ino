#include <VL53L0X.h>
#include <Wire.h>

// variaveis principais
#define START_BUTTON 6
#define STOP_BUTTON 5

// usar isso pra saber a distância do sensor até o fundo da plataforma e saber, assim, se a medida deve ou não ser descartada, por exemplo.
#define BACK_DISTANCE 7000 // 800 (distância em mm) - 100 (margem em mm)
int baseSteps = 0;

VL53L0X sensor;

class Point {
  public:
    float x = 0;
    float y = 0;
    float z = 0;
    
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

void setup()
{
  // inicio definicao pinos
  pinMode(START_BUTTON, INPUT_PULLUP); // botao de inicio // quero botar um pulldown
  pinMode(STOP_BUTTON, INPUT_PULLUP);  // botao stop
  // fim definicao pinos
  
  Serial.begin(9600);
  Wire.begin();

  // https://github.com/pololu/vl53l0x-arduino#:~:text=uint16_t%20readRangeContinuousMillimeters()
  // falar sobre o pq desses numeros no artigo
  sensor.init();
  sensor.setTimeout(500);
  sensor.setMeasurementTimingBudget(200000);
  sensor.startContinuous();
}

float getX() { 
  return 1; 
}

float getY() { 
  return 1;
}

float getZ()
{
  return sensor.readRangeContinuousMillimeters();
}

Point measure3DPoint()
{
  
  float x = getX();
  float y = getY();
  float z = getZ();

  Point point;

  point.x = x;
  point.y = y;
  point.z = z;

  return point;
}

void getColumn()
{
  // levar sensor para altura 0
  
  bool heightLimit = false; 
  
  while (true)
  {
    Point point = measure3DPoint();

    // update heightLimit (basicamente, pegar altura do sensor e ver se está no limite da plataforma)
    
    if (point.z >= BACK_DISTANCE - 10 || heightLimit) // -10 de margem de erro; o certo, a partir do momento que eu tiver a altura do sensor, é fazer um && não um ||, pq ai o objeto pode ter um furo no meio
    {
      Serial.println(point.z);
      break;
    }

    // printa os dados no serial, assim, o outro programa só recebe os pontos do objeto
    point.toSerial();
    
    // sobe o sensor um pouco
  }
}

void rotateBaseOneStep() {
  baseSteps += 1;
  // girar a base um pouquinho
}

void scanObject() {
  while (true)
  {
    getColumn();
    rotateBaseOneStep();
    // tem que ver quantas rotações vamos fazer na base do objeto
    // se total de giros na base for concluido, parar a medicao
    if (baseSteps == 36) {
      break;
    }
    // se o botao stop for presisonado
    if (digitalRead(STOP_BUTTON))
    {
      // break;
    }
  };
}

void loop()
{
  if (not digitalRead(START_BUTTON)) {
    scanObject();
    baseSteps = 0;
    Serial.println("");
    Serial.println("done");
  }
}

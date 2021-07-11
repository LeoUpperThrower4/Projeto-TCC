#include "Adafruit_VL53L0X.h"
#include <Wire.h>

// usar isso pra saber a distância do sensor até o fundo da plataforma e saber, assim, se a medida deve ou não ser descartada, por exemplo.
#define float BACK_DISTANCE = 500 - 100; // 500 (distância em mm) - 100 (margem em mm)
float pointCloud[10000];                 // número de pontos na núvem de pontos
int currentPoint = 0;
int baseSteps = 0;


Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setPins() {
  
}

void setup()
{
  Serial.begin(9600);
  // defining I2C ports
  Wire.begin(23, 22);

  if (!lox.begin())
  {
    Serial.println("Failed to boot VL53L0X. Resetting ESP32 in 5 seconds");
    delay(5000);
    ESP.restart();
  }

  setPins()
}

float getX() { return random(500); }

float getY() { return random(500); }

// return, in millimeters, the mean distance of 3 measures
float getZ()
{
  VL53L0X_RangingMeasurementData_t measure1, measure2, measure3;

  lox.rangingTest(&measure1, false);
  Serial.println(measure1.RangeMilliMeter);
  delay(1);
  lox.rangingTest(&measure2, false);
  Serial.println(measure2.RangeMilliMeter);
  delay(1);
  lox.rangingTest(&measure3, false);
  Serial.println(measure3.RangeMilliMeter);

  float meanMeasure = (measure1.RangeMilliMeter + measure2.RangeMilliMeter + measure3.RangeMilliMeter) / 3;

  if (measure1.RangeStatus != 4 && measure2.RangeStatus != 4 && measure3.RangeStatus != 4)
  {
    return meanMeasure;
  }
  else
  {
    return 0;
  }
}

float *create3DPointFromMeasurement()
{
  float *point = new float[3];
  point[0] = getX();
  point[1] = getY();
  point[2] = getZ();
  return point;
}

void getColumn()
{
  // sensor comeca embaixo
  bool heightLimit = false;   // limite fisico da plataforma (para objetos maiores que o limite). isso tem q ver quando a gente conseguir saber como faz pra pegar a altura do motor

  while (true)
  {
    float *point = create3DPointFromMeasurement();

    // update heightLimit
    
    if (point[2] >= BACK_DISTANCE || heightLimit)
    {
      break;
    }

    pointCloud[currentPoint] = point;
    currentPoint += 1;
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
    if (baseSteps == 36) {
      break;
    }
  }
}

void saveObject() {
  // save object as .stl file
}

bool buttonStart = true                // por enquanto é teste, tem que implementar melhor depois.
void loop()
{
  // update buttonStart
  if (buttonStart) {
    scanObject();
    saveObject();
    buttonStart = false;
  }
}

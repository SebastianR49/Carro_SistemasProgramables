#ifndef OBJECT_TRACKER_H
#define OBJECT_TRACKER_H

#include <Arduino.h>

// Constantes para el seguimiento de objetos
const int MAX_DISTANCE = 200;      // Distancia máxima de detección en cm
const int MIN_VALID_DISTANCE = 5;  // Distancia mínima válida en cm
const int TARGET_DISTANCE = 20;    // Distancia objetivo al objeto en cm
const int DISTANCE_TOLERANCE = 5;  // Tolerancia en la distancia
const int ROTATION_TIME = 100;     // Tiempo de rotación en ms
const int SCAN_DELAY = 50;        // Retardo entre mediciones

class ObjectTracker {
private:
    int trigPin;
    int echoPin;
    int motorLeftDir;
    int motorLeftSpeed;
    int motorRightDir;
    int motorRightSpeed;
    
    float minDistance;
    unsigned long lastScanTime;
    unsigned long rotationStartTime;
    int scanPhase;  // 0-7 para 8 direcciones
    
    enum State {
        SCANNING,
        FOLLOWING
    } state;

    float measureDistance() {
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        return pulseIn(echoPin, HIGH) / 58.00;
    }

    void moveForward(int speed) {
        digitalWrite(motorLeftDir, HIGH);
        analogWrite(motorLeftSpeed, speed);
        digitalWrite(motorRightDir, LOW);
        analogWrite(motorRightSpeed, speed);
    }

    void moveBackward(int speed) {
        digitalWrite(motorLeftDir, LOW);
        analogWrite(motorLeftSpeed, speed);
        digitalWrite(motorRightDir, HIGH);
        analogWrite(motorRightSpeed, speed);
    }

    void rotateLeft(int speed) {
        digitalWrite(motorLeftDir, LOW);
        analogWrite(motorLeftSpeed, speed);
        digitalWrite(motorRightDir, LOW);
        analogWrite(motorRightSpeed, speed);
    }

    void rotateRight(int speed) {
        digitalWrite(motorLeftDir, HIGH);
        analogWrite(motorLeftSpeed, speed);
        digitalWrite(motorRightDir, HIGH);
        analogWrite(motorRightSpeed, speed);
    }

    void stop() {
        analogWrite(motorLeftSpeed, 0);
        analogWrite(motorRightSpeed, 0);
    }

public:
    ObjectTracker(int _trigPin, int _echoPin, 
                 int _motorLeftDir, int _motorLeftSpeed,
                 int _motorRightDir, int _motorRightSpeed) {
        trigPin = _trigPin;
        echoPin = _echoPin;
        motorLeftDir = _motorLeftDir;
        motorLeftSpeed = _motorLeftSpeed;
        motorRightDir = _motorRightDir;
        motorRightSpeed = _motorRightSpeed;
        
        state = SCANNING;
        minDistance = MAX_DISTANCE;
        lastScanTime = 0;
        rotationStartTime = 0;
        scanPhase = 0;
        
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
    }

    void update() {
        switch (state) {
            case SCANNING:
                performScan();
                break;
            case FOLLOWING:
                followTarget();
                break;
        }
    }

private:
    void performScan() {
        if (millis() - lastScanTime > SCAN_DELAY) {
            lastScanTime = millis();
            
            // Tomar medida en la posición actual
            float distance = measureDistance();
            
            // Si encontramos un objeto más cercano, actualizar la distancia mínima
            if (distance > MIN_VALID_DISTANCE && distance < minDistance) {
                minDistance = distance;
                // Si el objeto está lo suficientemente cerca, comenzar a seguirlo
                if (minDistance < MAX_DISTANCE / 2) {
                    state = FOLLOWING;
                    stop();
                    return;
                }
            }
            
            // Rotar para escanear en diferentes direcciones
            if (millis() - rotationStartTime > ROTATION_TIME) {
                rotationStartTime = millis();
                scanPhase = (scanPhase + 1) % 8;  // 8 direcciones diferentes
                
                if (scanPhase == 0) {
                    // Reiniciar escaneo
                    minDistance = MAX_DISTANCE;
                }
                
                // Rotar 45 grados
                rotateRight(100);
            }
        }
    }
    
    void followTarget() {
        float distance = measureDistance();
        
        // Si perdimos el objeto, volver a escanear
        if (distance > MAX_DISTANCE || distance < MIN_VALID_DISTANCE) {
            stop();
            state = SCANNING;
            minDistance = MAX_DISTANCE;
            scanPhase = 0;
            return;
        }
        
        // Ajustar la distancia al objeto
        if (distance > TARGET_DISTANCE + DISTANCE_TOLERANCE) {
            moveForward(100);  // Moverse hacia el objeto
        } else if (distance < TARGET_DISTANCE - DISTANCE_TOLERANCE) {
            moveBackward(80);  // Alejarse del objeto
        } else {
            stop();  // Mantener la posición
        }
    }
};

#endif
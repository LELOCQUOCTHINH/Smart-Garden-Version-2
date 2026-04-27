#include "tinyml.h"

// Globals, for the convenience of one-shot setup.
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; // Adjust size based on your model
    uint8_t tensor_arena[kTensorArenaSize];
} // namespace

void setupTinyML()
{
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); // g_model_data is from model_data.h
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void tiny_ml_task(void *pvParameters)
{

    setupTinyML();

    while (1)
    {
        // Prepare input data (e.g., sensor readings)
        // For a simple example, let's assume a single float input
        if(xSemaphoreTake(xMutexTempHumi, (TickType_t)10) == pdTRUE) {
            // We can read temperature and humidity safely
            input->data.f[0] = glob_temperature;
            input->data.f[1] = glob_humidity;
            xSemaphoreGive(xMutexTempHumi);
        } else {
            Serial.println("⚠️ ERROR: cannot get Mutex, skip updating temperature and humidity for TinyML!");
            // If we can't get the mutex, we can choose to skip this inference or handle it as needed
        }
        
        if(xSemaphoreTake(xMutexSoilMoisture, (TickType_t)10) == pdTRUE) {
            // We can read soil moisture safely
            input->data.f[2] = glob_soil_moisture;
            xSemaphoreGive(xMutexSoilMoisture);
        } else {
            Serial.println("⚠️ ERROR: cannot get Mutex, skip updating soil moisture for TinyML!");
            // If we can't get the mutex, we can choose to skip this inference or handle it as needed
        }

        // Run inference
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk)
        {
            error_reporter->Report("Invoke failed");
            return;
        }

        // Get and process output
        float result = output->data.f[0];
        Serial.print("Inference result: ");
        Serial.println(result);

        if(result > 0.8) { // Assuming binary classification with threshold at 0.8
            // Serial.println("Anomaly detected!");
            sendToOLED(0, 50, "AI Inference:Abnormal");
            // sendToOLED(0, 60, "Relay: ON");
        } else {
            // Serial.println("No anomaly.");
            sendToOLED(0, 50, "AI Inference: Normal ");
            // sendToOLED(0, 60, "Relay: OFF");
        }

        vTaskDelay(5000);
    }
}
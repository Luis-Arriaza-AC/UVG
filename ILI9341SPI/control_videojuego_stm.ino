#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// UART2 del ESP32 clasico: RX=GPIO16, TX=GPIO17.
// Para este proyecto solo se conecta TX del ESP32 al RX de USART3 de la STM32.
HardwareSerial StmSerial(2);
constexpr int STM_RX_PIN = 16;  // No se usa, pero Serial2 requiere definirlo.
constexpr int STM_TX_PIN = 17;
constexpr uint32_t STM_BAUD = 115200;

struct ControlState {
    bool left;
    bool right;
    bool action;
};

ControlState sentState = {false, false, false};

void sendEvent(char button, bool pressed) {
    const uint8_t packet[2] = {
        static_cast<uint8_t>(button),
        static_cast<uint8_t>(pressed ? '1' : '0')
    };
    StmSerial.write(packet, sizeof(packet));
}

void releaseAllControls() {
    if (sentState.left) sendEvent('I', false);
    if (sentState.right) sendEvent('D', false);
    if (sentState.action) sendEvent('A', false);
    sentState = {false, false, false};
}

void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            myControllers[i] = ctl;
            Serial.printf("Control conectado, indice=%d, modelo=%s\n",
                          i, ctl->getModelName().c_str());
            return;
        }
    }
    Serial.println("Control conectado, pero no hay espacio disponible");
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            myControllers[i] = nullptr;
            releaseAllControls();
            Serial.printf("Control desconectado del indice=%d\n", i);
            return;
        }
    }
}

void processGamepad(ControllerPtr ctl) {
    const uint8_t dpad = ctl->dpad();

    ControlState current = {
        (dpad & DPAD_LEFT) != 0,
        (dpad & DPAD_RIGHT) != 0,
        ctl->a() || ctl->b()
    };

    if (current.left != sentState.left)
        sendEvent('I', current.left);
    if (current.right != sentState.right)
        sendEvent('D', current.right);
    if (current.action != sentState.action)
        sendEvent('A', current.action);

    sentState = current;
}

void processControllers() {
    // Se utiliza el primer gamepad conectado.
    for (ControllerPtr ctl : myControllers) {
        if (ctl && ctl->isConnected() && ctl->hasData() && ctl->isGamepad()) {
            processGamepad(ctl);
            return;
        }
    }
}

void setup() {
    Serial.begin(115200);  // Monitor serial por USB.
    StmSerial.begin(STM_BAUD, SERIAL_8N1, STM_RX_PIN, STM_TX_PIN);

    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys();
    BP32.enableVirtualDevice(false);

    // Sincroniza el estado inicial de la STM32.
    sendEvent('I', false);
    sendEvent('D', false);
    sendEvent('A', false);
}

void loop() {
    if (BP32.update())
        processControllers();

    delay(10);
}

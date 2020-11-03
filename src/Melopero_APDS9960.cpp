//Author: Leonardo La Rocca

#include "Melopero_APDS9960.h"

Melopero_APDS9960::Melopero_APDS9960(uint8_t i2cAddr){
    i2cAddress = i2cAddr;
}

//=========================================================================
//    I2C functions
//=========================================================================

int8_t Melopero_APDS9960::read(uint8_t registerAddress, uint8_t* buffer, uint8_t amount){
    Wire.beginTransmission(i2cAddress);
    Wire.write(registerAddress);
    uint8_t i2cStatus = Wire.endTransmission();
    if (i2cStatus != 0) return I2C_ERROR;

    uint32_t dataIndex = 0;
    do {
        uint8_t request = amount > 32 ? 32 : amount;
        Wire.requestFrom(i2cAddress, request);
        for (uint8_t i = 0; i < request; i++){
            if (Wire.available()){
                buffer[dataIndex] = Wire.read();
                dataIndex++;
                amount--;
            }
            else {
                return I2C_ERROR;
            }
        }
    }
    while (amount > 0);
    return NO_ERROR;
}
    
int8_t Melopero_APDS9960::write(uint8_t registerAddress, uint8_t* values, uint8_t len){
    Wire.beginTransmission(i2cAddress);
    Wire.write(registerAddress);
    Wire.write(values, len);
    uint8_t i2cStatus = Wire.endTransmission();
    if (i2cStatus != 0)
        return I2C_ERROR;
    else 
        return NO_ERROR;
}

int8_t Melopero_APDS9960::andOrRegister(uint8_t registerAddress, uint8_t andValue, uint8_t orValue){
    uint8_t value = 0;
    int8_t status = read(registerAddress, &value, 1);
    if (status != NO_ERROR) return status;
    value &= andValue;
    value |= orValue;
    return write(registerAddress, &value, 1);
}

int8_t Melopero_APDS9960::addressAccess(uint8_t registerAddress){
    Wire.beginTransmission(i2cAddress);
    Wire.write(registerAddress);
    uint8_t i2cStatus = Wire.endTransmission();
    if (i2cStatus != 0)
        return I2C_ERROR;
    else 
        return NO_ERROR;
}

// =========================================================================
//     Device Methods
// =========================================================================

int8_t wakeUp(bool wakeUp){
    int8_t status = NO_ERROR;
    status = andOrRegister(ENABLE_REG_ADDRESS, ((uint8_t) wakeUp) | 0xFE, (uint8_t) wakeUp);
    delay(10);
    return status;
}

int8_t Melopero_APDS9960::reset();

int8_t Melopero_APDS9960::enableAllEnginesAndPowerUp(bool enable){
    uint8_t value = enable ? 0b01001111 : 0;
    int8_t status = write(ENABLE_REG_ADDRESS, &value, 1);
    delay(10);
    return status;
}

int8_t Melopero_APDS9960::setSleepAfterInterrupt(bool enable){
    int8_t status = NO_ERROR;
    status = andOrRegister(CONFIG_3_REG_ADDRESS, (enable << 4) | 0xEF, enable << 4);
    return status;
}

int8_t Melopero_APDS9960::setLedDrive(uint8_t ledDrive){
    if (!(LED_DRIVE_100_mA <= ledDrive && ledDrive <= APDS_9960.LED_DRIVE_12_5_mA))
        return INVALID_ARGUMENT;

    return andOrRegister(CONTROL_1_REG_ADDRESS, (ledDrive << 6) | 0x3F, ledDrive << 6));
}

int8_t Melopero_APDS9960::setLedBoost(uint8_t ledBoost){
        if (!(LED_BOOST_100 <= ledBoost && ledBoost <= LED_BOOST_300))
            return INVALID_ARGUMENT;

        return andOrRegister(CONFIG_2_REG_ADDRESS, (ledBoost << 4) | 0xCF, (ledBoost << 4));
}

int8_t Melopero_APDS9960::updateStatus(){
    // TODO: add simple way to retrieve status flags:
    // Clear photodiode saturation (0x80) | Proximity/Gesture saturation (0x40) | Proximity interrupt (0x20) | ALS Interrupt (0x10)
    // | Gesture Interrupt (0x04) | Proximity valid (0x02) | ALS valid (0x01)
    return read(STATUS_REG_ADDRESS, &deviceStatus, 1);
}

// =========================================================================
//     Proximity Engine Methods
// =========================================================================

int8_t Melopero_APDS960::enableProximityEngine(bool enable){
    return andOrRegister(ENABLE_REG_ADDRESS, (enable << 2) | 0xFB, enable << 2);
}

int8_t Melopero_APDS9960::enableProximityInterrupts(bool enable){
    return andOrRegister(ENABLE_REG_ADDRESS, (enable << 5) | 0xDF, enable << 5);
}

int8_t Melopero_APDS9960::enableProximitySaturationInterrupts(bool enable){
    return andOrRegister(CONFIG_2_REG_ADDRESS, (enable << 7) | 0x7F, enable << 7);
}

// Interrupts are cleared by “address accessing” the appropriate register. This is special I2C transaction
// consisting of only two bytes: chip address with R/W = 0, followed by a register address.
int8_t Melopero_APDS9960::clearProximityInterrupts(){ 
    return addressAccess(PROXIMITY_INT_CLEAR_REG_ADDRESS);
}

int8_t Melopero_APDS9960::setProximityGain(uint8_t proxGain){
    if (!(PROXIMITY_GAIN_1X <= proxGain && proxGain <= PROXIMITY_GAIN_8X)
        return INVALID_ARGUMENT;

    return andOrRegister(CONTROL_1_REG_ADDRESS, (proxGain << 2) | 0xF3, proxGain << 2);
}

int8_t Melopero_APDS9960::setProximityInterruptThresholds(uint8_t lowThr, uint8_t highThr){
    int8_t status = NO_ERROR;
    status = write(PROX_INT_LOW_THR_REG_ADDRESS, &lowThr, 1);
    if (status != NO_ERROR) return status;
    status = write(PROX_INT_HIGH_THR_REG_ADDRESS, &highThr, 1);
    return status;
}

int8_t Melopero_APDS9960::setProximityInterruptPersistence(uint8_t persistence){
    if (!(0 <= persistence && persistence <= 15))
        return INVALID_ARGUMENT;

    return andOrRegister(INTERRUPT_PERSISTANCE_REG_ADDRESS, (persistence << 4) | 0x0F, persistence << 4);
}

int8_t Melopero_APDS9960::setProximityPulseCountAndLength(uint8_t pulseCount, uint8_t pulseLength){
    if (!(1 <= pulseCount <= 64))
        return INVALID_ARGUMENT;
    if (!(PULSE_LEN_4_MICROS <= pulseLength <= PULSE_LEN_32_MICROS))
        return INVALID_ARGUMENT;       

    uint8_t regValue = pulseLength << 6;
    regValue |= pulseCount - 1
    return write(PROX_PULSE_COUNT_REG_ADDRESS, &regValue, 1);
}

int8_t Melopero_APDS9960::setProximityOffset(int8_t upRightOffset, int8_t downLeftOffset){
    int8_t status = NO_ERROR;
    uint8_t upRightRegValue = upRightOffset < 0 ? 0x80 | ((uint8_t) (-upRightOffset)) : (uint8_t) upRightOffset;
    uint8_t downLeftRegValue = downLeftOffset < 0 ? 0x80 | ((uint8_t) (-downLeftOffset)) : (uint8_t) downLeftOffset;

    status = write(PROX_UP_RIGHT_OFFSET_REG_ADDRESS, &upRightRegValue, 1);
    if (status != NO_ERROR) return status;
    status = write(PROX_DOWN_LEFT_OFFSET_REG_ADDRESS, &downLeftRegValue, 1);
    return status;
}

int8_t Melopero_APDS9960::disablePhotodiodes(bool mask_up, bool mask_down, bool mask_left, bool mask_right, bool proximity_gain_compensation){
    uint8_t and_flag = 0xD0 | (proximity_gain_compensation << 5) | (mask_up << 3) | (mask_down << 2) | (mask_left << 1) | ((uint8_t) mask_right);
    uint8_t or_flag = (proximity_gain_compensation << 5) | (mask_up << 3) | (mask_down << 2) | (mask_left << 1) | ((uint8_t) mask_right);

    return andOrRegister(CONFIG_3_REG_ADDRESS, and_flag, or_flag);
}

int8_t Melopero_APDS9960::updateProximityData(){
    return read(PROX_DATA_REG_ADDRESS, &proximityData, 1);
}

// =========================================================================
//     ALS Engine Methods
// =========================================================================

int8_t Melopero_APDS9960::enableAlsEngine(bool enable = true){
    return andOrRegister(ENABLE_REG_ADDRESS, (enable << 1) | 0xFD, enable << 1);
}

int8_t Melopero_APDS9960::enableAlsInterrupts(bool enable = true){
    return andOrRegister(ENABLE_REG_ADDRESS, (enable << 4) | 0xEF, enable << 4);
}

int8_t Melopero_APDS9960::enableAlsSaturationInterrupts(bool enable = true){
    return andOrRegister(CONFIG_2_REG_ADDRESS, (enable << 6) | 0xBF, enable << 6);
}

// Interrupts are cleared by “address accessing” the appropriate register. This is special I2C transaction
// consisting of only two bytes: chip address with R/W = 0, followed by a register address.
int8_t Melopero_APDS9960::clearAlsInterrupts(){
    return addressAccess(ALS_INT_CLEAR_REG_ADDRESS);
}

int8_t Melopero_APDS9960::setAlsGain(uint8_t als_gain){
    if (!(ALS_GAIN_1X <= als_gain && als_gain <= ALS_GAIN_64X))
        return INVALID_ARGUMENT;
    
    return andOrRegister(CONTROL_1_REG_ADDRESS, (als_gain) | 0xFC, als_gain);
}

int8_t Melopero_APDS9960::setAlsThresholds(uint16_t low_thr, uint16_t high_thr){
    uint8_t buffer[4];
    buffer[0] = low_thr & 0xFF;
    buffer[1] = low_thr >> 8;
    buffer[2] = high_thr & 0xFF;
    buffer[3] = high_thr >> 8;
    return write(ALS_INT_LOW_THR_LOW_BYTE_REG_ADDRESS, buffer, 4);
}

int8_t Melopero_APDS9960::setAlsInterruptPersistence(uint8_t persistence){
    if (persistence > 15)
        return INVALID_ARGUMENT;
    
    return andOrRegister(INTERRUPT_PERSISTANCE_REG_ADDRESS, persistence | 0xF0, persistence);
}

int8_t Melopero_APDS9960::setAlsIntegrationTime(float wtime){
    if (!(2.78f <= wtime && wtime <= 712f))
        return INVALID_ARGUMENT;

    uint8_t value = 256 - (int)( wtime / 2.78f);
    return write(ALS_ATIME_REG_ADDRESS, &value, 1);
}

int8_t Melopero_APDS9960::updateSaturation(){ 
        uint8_t reg_value = 0;
        int8_t status = read(ALS_ATIME_REG_ADDRESS, &reg_value, 1);
        if (status != NO_ERROR) return status;

        uint8_t cycles = (256 - reg_value) * 1025;
        alsSaturation = cycles < 65535 ? cycles : 65535;
        return NO_ERROR;
}

int8_t Melopero_APDS9960::updateColorData(){
    uint8_t color_buffer[8] = {0};
    int8_t status = read(CLEAR_DATA_LOW_BYTE_REG_ADDRESS, &color_buffer, 8);
    if (status != NO_ERROR) return status;

    clear = ((uint16_t) color_buffer[1]) << 8 | (uint16_t) color_buffer[0];
    red = ((uint16_t) color_buffer[3]) << 8 | (uint16_t) color_buffer[2];
    green = ((uint16_t) color_buffer[5]) << 8 | (uint16_t) color_buffer[4];
    blue = ((uint16_t) color_buffer[7]) << 8 | (uint16_t) color_buffer[6];
    return NO_ERROR;
}

// =========================================================================
//     Gestures Engine Methods
// =========================================================================

int8_t Melopero_APDS9960::enableGesturesEngine(bool enable){
    int8_t status = enableProximityEngine();
    if (status != NO_ERROR) return status;
    return andOrRegister(ENABLE_REG_ADDRESS, (enable << 6) 0xBF, enable << 6);
}

int8_t Melopero_APDS9960::enterImmediatelyGestureEngine(){
    return andOrRegister(GESTURE_CONFIG_4_REG_ADDRESS, 0xFF, 1);
}

int8_t Melopero_APDS9960::exitGestureEngine(){
    return andOrRegister(GESTURE_CONFIG_4_REG_ADDRESS, 0xFE, 0);
}

int8_t Melopero_APDS9960::setGestureProxEnterThreshold(uint8_t enter_thr){
   return write(GESTURE_PROX_ENTER_THR_REG_ADDRESS, &enter_thr, 1);
}

int8_t Melopero_APDS9960::setGestureExitThreshold(uint8_t exit_thr){
    return write(GESTURE_EXIT_THR_REG_ADDRESS, &exit_thr, 1);
}

int8_t Melopero_APDS9960::setGestureExitMask(bool mask_up, bool mask_down, bool mask_left, bool mask_right){
    uint8_t flag = (mask_up << 5) | (mask_down << 4) | (mask_left << 3) | (mask_right << 2);
    return andOrRegister(GESTURE_CONFIG_1_REG_ADDRESS, flag | 0xC3, flag);
}

int8_t Melopero_APDS9960::setGestureExitPersistence(uint8_t persistence){
    if (!(EXIT_AFTER_1_GESTURE_END <= persistence && persistence <= EXIT_AFTER_7_GESTURE_END))
        return INVALID_ARGUMENT;
    
    return andOrRegister(GESTURE_CONFIG_1_REG_ADDRESS, (persistence) | 0xFC, persistence);
}

int8_t Melopero_APDS9960::setGestureGain(uint8_t gesture_gain){
    if (!(PROXIMITY_GAIN_1X <= gesture_gain && gesture_gain <= PROXIMITY_GAIN_8X))
        return INVALID_ARGUMENT;

    return andOrRegister(GESTURE_CONFIG_2_REG_ADDRESS, (gesture_gain << 5) | 0x9F, gesture_gain << 5);
}

int8_t Melopero_APDS9960::setGestureLedDrive(uint8_t led_drive){
    if (!(LED_DRIVE_100_mA <= led_drive && led_drive <= LED_DRIVE_12_5_mA))
        return INVALID_ARGUMENT;
    
    return andOrRegister(GESTURE_CONFIG_2_REG_ADDRESS, (led_drive << 3) | 0x7E, led_drive << 3);
}

int8_t Melopero_APDS9960::setGestureWaitTime(uint8_t wait_time){
    if (!(GESTURE_WAIT_0_MILLIS <= wait_time && wait_time <= GESTURE_WAIT_39_2_MILLIS))
        return INVALID_ARGUMENT;

    return andOrRegister(GESTURE_CONFIG_2_REG_ADDRESS, wait_time | 0xF8, wait_time);
}

int8_t Melopero_APDS9960::setGestureOffsets(int8_t up_offset, int8_t down_offset, int8_t left_offset, int8_t right_offset){
    uint8_t offsets[4] = {0};
    offset[0] = up_offset < 0 ? 0x80 | ((uint8_t) -up_offset) : up_offset;
    offset[1] = down_offset < 0 ? 0x80 | ((uint8_t) -down_offset) : down_offset;
    offset[2] = left_offset < 0 ? 0x80 | ((uint8_t) -left_offset) : left_offset;
    offset[3] = right_offset < 0 ? 0x80 | ((uint8_t) -right_offset) : right_offset;

    return write(GESTURE_OFFSET_UP_REG_ADDRESSES, offsets, 4);
}

int8_t Melopero_APDS9960::setGesturePulseCountAndLength(uint8_t pulse_count, uint8_t pulse_length){
    if (!(1 <= pulse_count && pulse_count <= 64))
        return INVALID_ARGUMENT;
    if (!(PULSE_LEN_4_MICROS <= pulse_length && pulse_length <= PULSE_LEN_32_MICROS))
        return INVALID_ARGUMENT;

    uint8_t reg_value = (pulse_count - 1) | (pulse_length << 6);
    return write(GESTURE_PULSE_COUNT_AND_LEN_REG_ADDRESS, &reg_value, 1);
}

int8_t Melopero_APDS9960::setActivePhotodiodesPairs(bool up_down_active, bool right_left_active){
    uint8_t flag = (right_left_active << 1 | (uint8_t) up_down_active);
    return andOrRegister(GESTURE_CONFIG_3_REG_ADDRESS, flag | 0xFC, flag);
}

int8_t Melopero_APDS9960::enableGestureInterrupts(bool enable_interrupts = true){
    return andOrRegister(GESTURE_CONFIG_4_REG_ADDRESS, (enable_interrupts << 1) | 0xFD, enable_interrupts << 1);
}

int8_t Melopero_APDS9960::setGestureFifoThreshold(uint8_t fifo_thr){
    if (!(FIFO_INT_AFTER_1_DATASET <= fifo_thr && fifo_thr <= FIFO_INT_AFTER_16_DATASETS))
        return INVALID_ARGUMENT;

    return andOrRegister(GESTURE_CONFIG_1_REG_ADDRESS, (fifo_thr << 6) | 0x3F, fifo_thr << 6);
}

int8_t Melopero_APDS9960::clearGestureEngineInterrupts(){
    uint8_t flag = (1 << 2);
    return andOrRegister(GESTURE_CONFIG_4_REG_ADDRESS, flag | 0xFB, flag);
}

int8_t Melopero_APDS9960::checkGestureEngineRunning(){
    uint8_t reg_value = 0;
    int8_t status = read(GESTURE_CONFIG_4_REG_ADDRESS, &reg_value, 1);
    if (status != NO_ERROR) return status;

    gestureEngineRunning = (bool) reg_value & 0x01;
    return NO_ERROR;
}

int8_t Melopero_APDS9960::updateNumberOfDatasetsInFifo(){
    return read(GESTURE_FIFO_LEVEL_REG_ADDRESS, &datasetsInFifo, 1);    
}

int8_t Melopero_APDS9960::updateGestureStatus(){
    uint8_t reg_value = 0;
    int8_t status = read(GESTURE_STATUS_REG_ADDRESS, &reg_value, 1);
    if (status != NO_ERROR) return status;

    gestureFifoOverflow = (bool) (reg_value & 0x02);
    gestureFifoHasData = (bool) (reg_value & 0x01);
    return NO_ERROR;
}

int8_t Melopero_APDS9960::updateGestureData(){
    return read(GESTURE_FIFO_UP_REG_ADDRESS, gestureData, 4);       
}

// =========================================================================
//     Wait Engine Methods
// =========================================================================

int8_t Melopero_APDS9960::enableWaitEngine(bool enable){
    return andOrRegister(ENABLE_REG_ADDRESS, (enable << 3) | 0xF7, enable << 3);
}

int8_t Melopero_APDS9960::setWaitTime(float wtime, bool long_wait){
    if (!(2.78f <= wtime && wtime <= 712f))
        return INVALID_ARGUMENT;

    // long_wait
    int8_t status = andOrRegister(CONFIG_1_REG_ADDRESS, (long_wait << 1) | 0xFD, long_wait << 1);
    if (status != NO_ERROR) return status;
    // wtime
    uint8_t reg_value = 256 - ((int) (wtime / 2.78f));
    return write(WAIT_TIME_REG_ADDRESS, &reg_value, 1);
}
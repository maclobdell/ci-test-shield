// check if I2C is supported on this device
#if !DEVICE_I2C
  #error [NOT_SUPPORTED] I2C not supported on this platform, add 'DEVICE_I2C' deffinition to your platform.
#endif

#include "mbed.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"
#include "LM75B.h"
#include <I2CEeprom.h>

using namespace utest::v1;

// Fill array with random characters
void init_string(char* buffer, int len){
    int x = 0;
    for(x = 0; x < len; x++){
        buffer[x] = 'A' + (rand() % 26);
    }
    buffer[len-1] = 0; // add \0 to end of string
    printf("\r\n****\r\nBuffer Len = `%d`, String = `%s`\r\n****\r\n",len,buffer);
}

// a test to see if the temperature can be read. A I2C failure returns a 0
template<PinName sda, PinName scl, int expected_temperature, int delta_in_temperature>
void test_lm75b(){
    LM75B  temperature(sda, scl);
    float temp = temperature.temp();
    printf("\r\n****\r\nTEST LM75B : Temperature Read = `%f`\r\n****\r\n",temp);
    TEST_ASSERT_MESSAGE(0 != temperature.open(),"Failed to open sensor");
    //TEST_ASSERT_MESSAGE(NULL != temperature.temp(),"Invalid value NULL returned");
    // TEST_ASSERT_MESSAGE(50 > temperature.temp(),"Its too Hot (>10C), Faulty Sensor?");
    // TEST_ASSERT_MESSAGE(0 < temperature.temp(),"Its Too Cold (<0C), Faulty Sensor?");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(delta_in_temperature, expected_temperature, temp,"Temperature is not within specified range");
}

// Template to write arbitrary data to arbitrary address and check the data is written correctly
template<PinName sda, PinName scl,int size_of_data, int address>
void flash_WR(){
    I2CEeprom memory(sda,scl,MBED_CONF_APP_I2C_EEPROM_ADDR,32,0);
    int num_read = 0;
    int num_written = 0;
    char test_string[size_of_data] = {0};
    char read_string[size_of_data] = {0};
    init_string(test_string,size_of_data); // populate test_string with random characters
    //printf("\r\n****\r\n Test String = `%s` \r\n****\r\n",test_string);

    num_written = memory.write(address,test_string,size_of_data);
    num_read = memory.read(address,read_string,size_of_data);

    printf("\r\n****\r\n Address = `%d`\r\n Len = `%d`\r\n Num Bytes Written = `%d` \r\n Num Bytes Read = `%d` \r\n Written String = `%s` \r\n Read String = `%s` \r\n****\r\n",address,size_of_data,num_written,num_read,test_string,read_string);

    TEST_ASSERT_MESSAGE(strcmp(test_string,read_string) == 0,"String Written != String Read");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(test_string,read_string,"String read does not match the string written");
    TEST_ASSERT_EQUAL_MESSAGE(num_written,num_read,"Number of bytes written does not match the number of bytes read");

}

// Test single byte R/W
template<PinName sda, PinName scl, int address>
void single_byte_WR(){
    I2CEeprom memory(sda,scl,MBED_CONF_APP_I2C_EEPROM_ADDR,32,0);
    char test = 'A' + rand()%26;
    char read;
    memory.write(address,test);
    memory.read(address,read);
    printf("\r\n****\r\n Read byte = %d \r\n Written Byte = %d \r\n****\r\n",&read,&test);

    TEST_ASSERT_EQUAL_MESSAGE(test,read,"Character Read does not equal character written!");
    TEST_ASSERT_MESSAGE(test == read, "character written does not match character read")

}

utest::v1::status_t test_setup(const size_t number_of_cases) {
    // Setup Greentea using a reasonable timeout in seconds
    GREENTEA_SETUP(40, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

// Handle test failures, keep testing, dont stop
utest::v1::status_t greentea_failure_handler(const Case *const source, const failure_t reason) {
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

// Test cases
Case cases[] = {
    Case("I2C -  LM75B Temperature Read",test_lm75b<MBED_CONF_APP_I2C_SDA,MBED_CONF_APP_I2C_SCL,25,20>,greentea_failure_handler),
    //Case("I2C -  EEProm WR 2Bytes",flash_WR<MBED_CONF_APP_I2C_SDA,MBED_CONF_APP_I2C_SCL,2,1>,greentea_failure_handler),
    Case("I2C -  EEProm WR 10Bytes",flash_WR<MBED_CONF_APP_I2C_SDA,MBED_CONF_APP_I2C_SCL,10,1>,greentea_failure_handler),
    Case("I2C -  EEProm WR 100 Bytes",flash_WR<MBED_CONF_APP_I2C_SDA,MBED_CONF_APP_I2C_SCL,100,1>,greentea_failure_handler),
    Case("I2C -  EEProm WR 100 Bytes",single_byte_WR<MBED_CONF_APP_I2C_SDA,MBED_CONF_APP_I2C_SCL,1>,greentea_failure_handler),
};

Specification specification(test_setup, cases);

// Entry point into the tests
int main() {
    return !Harness::run(specification);
}
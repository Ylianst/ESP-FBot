import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_BATTERY_LEVEL,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_DURATION,
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
    UNIT_WATT,
    UNIT_KILOWATT_HOURS,
    UNIT_MINUTE,
)
from . import fbot_ns, Fbot, CONF_FBOT_ID

DEPENDENCIES = ["fbot"]

CONF_INPUT_POWER = "input_power"
CONF_OUTPUT_POWER = "output_power"
CONF_SYSTEM_POWER = "system_power"
CONF_TOTAL_POWER = "total_power"
CONF_REMAINING_TIME = "remaining_time"
CONF_BATTERY_S1_LEVEL = "battery_s1_level"
CONF_BATTERY_S2_LEVEL = "battery_s2_level"
CONF_THRESHOLD_CHARGE = "threshold_charge"
CONF_THRESHOLD_DISCHARGE = "threshold_discharge"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_FBOT_ID): cv.use_id(Fbot),
        cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_BATTERY_S1_LEVEL): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_BATTERY_S2_LEVEL): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_INPUT_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_OUTPUT_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_SYSTEM_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TOTAL_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_REMAINING_TIME): sensor.sensor_schema(
            unit_of_measurement=UNIT_MINUTE,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_THRESHOLD_CHARGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_THRESHOLD_DISCHARGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_FBOT_ID])
    
    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(parent.set_battery_percent_sensor(sens))

    if CONF_BATTERY_S1_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_S1_LEVEL])
        cg.add(parent.set_battery_percent_s1_sensor(sens))

    if CONF_BATTERY_S2_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_S2_LEVEL])
        cg.add(parent.set_battery_percent_s2_sensor(sens))
    
    if CONF_INPUT_POWER in config:
        sens = await sensor.new_sensor(config[CONF_INPUT_POWER])
        cg.add(parent.set_input_power_sensor(sens))
    
    if CONF_OUTPUT_POWER in config:
        sens = await sensor.new_sensor(config[CONF_OUTPUT_POWER])
        cg.add(parent.set_output_power_sensor(sens))
    
    if CONF_SYSTEM_POWER in config:
        sens = await sensor.new_sensor(config[CONF_SYSTEM_POWER])
        cg.add(parent.set_system_power_sensor(sens))
    
    if CONF_TOTAL_POWER in config:
        sens = await sensor.new_sensor(config[CONF_TOTAL_POWER])
        cg.add(parent.set_total_power_sensor(sens))
    
    if CONF_REMAINING_TIME in config:
        sens = await sensor.new_sensor(config[CONF_REMAINING_TIME])
        cg.add(parent.set_remaining_time_sensor(sens))

    if CONF_THRESHOLD_CHARGE in config:
        sens = await sensor.new_sensor(config[CONF_THRESHOLD_CHARGE])
        cg.add(parent.set_threshold_charge_sensor(sens))

    if CONF_THRESHOLD_DISCHARGE in config:
        sens = await sensor.new_sensor(config[CONF_THRESHOLD_DISCHARGE])
        cg.add(parent.set_threshold_discharge_sensor(sens))

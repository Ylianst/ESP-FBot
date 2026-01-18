import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, DEVICE_CLASS_CONNECTIVITY, CONF_ICON
from . import fbot_ns, Fbot, CONF_FBOT_ID

DEPENDENCIES = ["fbot_dev"]

CONF_CONNECTED = "connected"
CONF_BATTERY_CONNECTED_S1 = "battery_connected_s1"
CONF_BATTERY_CONNECTED_S2 = "battery_connected_s2"
CONF_USB_ACTIVE = "usb_active"
CONF_DC_ACTIVE = "dc_active"
CONF_AC_ACTIVE = "ac_active"
CONF_LIGHT_ACTIVE = "light_active"

DEVICE_CLASS_OUTLET = "plug"
DEVICE_CLASS_LIGHT = "light"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_FBOT_ID): cv.use_id(Fbot),
        cv.Optional(CONF_CONNECTED): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_CONNECTIVITY,
        ),
        cv.Optional(CONF_BATTERY_CONNECTED_S1): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_CONNECTIVITY,
            icon="mdi:battery-plus",
        ),
        cv.Optional(CONF_BATTERY_CONNECTED_S2): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_CONNECTIVITY,
            icon="mdi:battery-plus",
        ),
        cv.Optional(CONF_USB_ACTIVE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_OUTLET,
        ),
        cv.Optional(CONF_DC_ACTIVE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_OUTLET,
        ),
        cv.Optional(CONF_AC_ACTIVE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_OUTLET,
        ),
        cv.Optional(CONF_LIGHT_ACTIVE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_LIGHT,
            icon="mdi:lightbulb",
        ),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_FBOT_ID])
    
    if CONF_CONNECTED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CONNECTED])
        cg.add(parent.set_connected_binary_sensor(sens))
    
    if CONF_BATTERY_CONNECTED_S1 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_BATTERY_CONNECTED_S1])
        cg.add(parent.set_battery_connected_s1_binary_sensor(sens))
    
    if CONF_BATTERY_CONNECTED_S2 in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_BATTERY_CONNECTED_S2])
        cg.add(parent.set_battery_connected_s2_binary_sensor(sens))
    
    if CONF_USB_ACTIVE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_USB_ACTIVE])
        cg.add(parent.set_usb_active_binary_sensor(sens))
    
    if CONF_DC_ACTIVE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_DC_ACTIVE])
        cg.add(parent.set_dc_active_binary_sensor(sens))
    
    if CONF_AC_ACTIVE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_AC_ACTIVE])
        cg.add(parent.set_ac_active_binary_sensor(sens))
    
    if CONF_LIGHT_ACTIVE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_LIGHT_ACTIVE])
        cg.add(parent.set_light_active_binary_sensor(sens))

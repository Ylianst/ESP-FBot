import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client
from esphome.const import CONF_ID

AUTO_LOAD = ["ble_client"]
DEPENDENCIES = ["ble_client"]
MULTI_CONF = True

CONF_FBOT_ID = "fbot_id"
CONF_POLLING_INTERVAL = "polling_interval"
CONF_SETTINGS_POLLING_INTERVAL = "settings_polling_interval"

fbot_ns = cg.esphome_ns.namespace("fbot")
Fbot = fbot_ns.class_(
    "Fbot", cg.Component, ble_client.BLEClientNode
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Fbot),
            cv.Optional(CONF_POLLING_INTERVAL, default="2s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_SETTINGS_POLLING_INTERVAL, default="60s"): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(ble_client.BLE_CLIENT_SCHEMA),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)
    
    cg.add(var.set_polling_interval(config[CONF_POLLING_INTERVAL]))
    cg.add(var.set_settings_polling_interval(config[CONF_SETTINGS_POLLING_INTERVAL]))

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client
from esphome.const import CONF_ID

AUTO_LOAD = ["ble_client"]
DEPENDENCIES = ["ble_client"]
MULTI_CONF = False

CONF_REGISTER = "register"
CONF_VALUE = "value"
CONF_HANDLE_MIN = "handle_min"
CONF_HANDLE_MAX = "handle_max"
CONF_BLAST_INTERVAL = "blast_interval"
CONF_BLAST_DURATION = "blast_duration"

fbot_rescue_ns = cg.esphome_ns.namespace("fbot_rescue")
FbotRescue = fbot_rescue_ns.class_(
    "FbotRescue", cg.Component, ble_client.BLEClientNode
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(FbotRescue),
            cv.Optional(CONF_REGISTER, default=2): cv.int_range(min=0, max=65535),
            cv.Optional(CONF_VALUE, default=0x00FF): cv.int_range(min=0, max=65535),
            cv.Optional(CONF_HANDLE_MIN, default=0x0002): cv.int_range(min=1, max=65535),
            cv.Optional(CONF_HANDLE_MAX, default=0x0020): cv.int_range(min=1, max=65535),
            cv.Optional(CONF_BLAST_INTERVAL, default="500ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_BLAST_DURATION, default="5s"): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(ble_client.BLE_CLIENT_SCHEMA),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)

    cg.add(var.set_register(config[CONF_REGISTER]))
    cg.add(var.set_value(config[CONF_VALUE]))
    cg.add(var.set_handle_min(config[CONF_HANDLE_MIN]))
    cg.add(var.set_handle_max(config[CONF_HANDLE_MAX]))
    cg.add(var.set_blast_interval(config[CONF_BLAST_INTERVAL]))
    cg.add(var.set_blast_duration(config[CONF_BLAST_DURATION]))

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID, CONF_ICON
from .. import fbot_ns, Fbot, CONF_FBOT_ID

DEPENDENCIES = ["fbot_dev"]

CONF_LIGHT_MODE = "light_mode"

FbotSelect = fbot_ns.class_("FbotSelect", select.Select, cg.Component)

# Light mode options mapping to register values
LIGHT_MODE_OPTIONS = {
    "Off": 0,
    "On": 1,
    "SOS": 2,
    "Flashing": 3,
}

SELECT_TYPES = {
    CONF_LIGHT_MODE: "light_mode",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_FBOT_ID): cv.use_id(Fbot),
        cv.Optional(CONF_LIGHT_MODE): select.select_schema(
            FbotSelect,
            icon="mdi:lightbulb-multiple",
        ),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_FBOT_ID])
    
    for key, select_type in SELECT_TYPES.items():
        if key in config:
            var = await select.new_select(config[key], options=list(LIGHT_MODE_OPTIONS.keys()))
            await cg.register_component(var, config[key])
            cg.add(var.set_parent(parent))
            cg.add(var.set_select_type(select_type))
            
            # Register select with parent to enable state synchronization
            if key == CONF_LIGHT_MODE:
                cg.add(parent.set_light_mode_select(var))

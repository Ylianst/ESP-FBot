import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, CONF_ICON
from .. import fbot_ns, Fbot, CONF_FBOT_ID

DEPENDENCIES = ["fbot_dev"]

CONF_USB = "usb"
CONF_DC = "dc"
CONF_AC = "ac"
CONF_LIGHT = "light"
CONF_AC_SILENT = "ac_silent"
CONF_KEY_SOUND = "key_sound"

FbotSwitch = fbot_ns.class_("FbotSwitch", switch.Switch, cg.Component)

SWITCH_TYPES = {
    CONF_USB: "usb",
    CONF_DC: "dc",
    CONF_AC: "ac",
    CONF_LIGHT: "light",
    CONF_AC_SILENT: "ac_silent",
    CONF_KEY_SOUND: "key_sound",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_FBOT_ID): cv.use_id(Fbot),
        cv.Optional(CONF_USB): switch.switch_schema(
            FbotSwitch,
            icon="mdi:usb-port",
        ),
        cv.Optional(CONF_DC): switch.switch_schema(
            FbotSwitch,
            icon="mdi:power-socket-de",
        ),
        cv.Optional(CONF_AC): switch.switch_schema(
            FbotSwitch,
            icon="mdi:power-plug",
        ),
        cv.Optional(CONF_LIGHT): switch.switch_schema(
            FbotSwitch,
            icon="mdi:lightbulb",
        ),
        cv.Optional(CONF_AC_SILENT): switch.switch_schema(
            FbotSwitch,
            icon="mdi:volume-off",
        ),
        cv.Optional(CONF_KEY_SOUND): switch.switch_schema(
            FbotSwitch,
            icon="mdi:volume-high",
        ),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_FBOT_ID])
    
    for key, switch_type in SWITCH_TYPES.items():
        if key in config:
            var = await switch.new_switch(config[key])
            await cg.register_component(var, config[key])
            cg.add(var.set_parent(parent))
            cg.add(var.set_switch_type(switch_type))
            
            # Register switch with parent to enable state synchronization
            if key == CONF_USB:
                cg.add(parent.set_usb_switch(var))
            elif key == CONF_DC:
                cg.add(parent.set_dc_switch(var))
            elif key == CONF_AC:
                cg.add(parent.set_ac_switch(var))
            elif key == CONF_LIGHT:
                cg.add(parent.set_light_switch(var))
            elif key == CONF_AC_SILENT:
                cg.add(parent.set_ac_silent_switch(var))
            elif key == CONF_KEY_SOUND:
                cg.add(parent.set_key_sound_switch(var))

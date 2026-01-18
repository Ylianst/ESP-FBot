import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ID,
    CONF_ICON,
    CONF_MIN_VALUE,
    CONF_MAX_VALUE,
    CONF_STEP,
    UNIT_PERCENT,
)
from .. import fbot_ns, Fbot, CONF_FBOT_ID

DEPENDENCIES = ["fbot_dev"]

CONF_THRESHOLD_CHARGE = "threshold_charge"
CONF_THRESHOLD_DISCHARGE = "threshold_discharge"

FbotNumber = fbot_ns.class_("FbotNumber", number.Number, cg.Component)

NUMBER_TYPES = {
    CONF_THRESHOLD_CHARGE: "threshold_charge",
    CONF_THRESHOLD_DISCHARGE: "threshold_discharge",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_FBOT_ID): cv.use_id(Fbot),
        cv.Optional(CONF_THRESHOLD_CHARGE): number.number_schema(
            FbotNumber,
            icon="mdi:battery-charging-100",
            unit_of_measurement=UNIT_PERCENT,
        ).extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=60): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.float_,
                cv.Optional(CONF_STEP, default=1): cv.float_,
            }
        ),
        cv.Optional(CONF_THRESHOLD_DISCHARGE): number.number_schema(
            FbotNumber,
            icon="mdi:battery-outline",
            unit_of_measurement=UNIT_PERCENT,
        ).extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=50): cv.float_,
                cv.Optional(CONF_STEP, default=1): cv.float_,
            }
        ),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_FBOT_ID])

    for key, number_type in NUMBER_TYPES.items():
        if key in config:
            var = await number.new_number(
                config[key],
                min_value=config[key][CONF_MIN_VALUE],
                max_value=config[key][CONF_MAX_VALUE],
                step=config[key][CONF_STEP],
            )
            await cg.register_component(var, config[key])
            cg.add(var.set_parent(parent))
            cg.add(var.set_number_type(number_type))
            
            # Register the number component with the parent so it can receive updates
            if key == CONF_THRESHOLD_CHARGE:
                cg.add(parent.set_threshold_charge_number(var))
            elif key == CONF_THRESHOLD_DISCHARGE:
                cg.add(parent.set_threshold_discharge_number(var))

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_INPUT,
    CONF_NUMBER,
    CONF_MODE,
    CONF_INVERTED,
    CONF_OUTPUT,
)

DEPENDENCIES = ["i2c"]
MULTI_CONF = True

tca6408a_ns = cg.esphome_ns.namespace("tca6408a")

TCA6408AComponent = tca6408a_ns.class_("TCA6408AComponent", cg.Component, i2c.I2CDevice)
TCA6408AGPIOPin = tca6408a_ns.class_("TCA6408AGPIOPin", cg.GPIOPin)

CONF_TCA6408A = "tca6408a"
CONF_DEFAULT_ON = "default_on"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(TCA6408AComponent),
            cv.Optional(CONF_TCA6408A, default=False): cv.boolean,
            cv.Optional(CONF_DEFAULT_ON, default=False): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x20))
#   .extend(i2c.i2c_device_schema(0x21))    
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    #cg.add(var.set_tca6408a(config[CONF_TCA6408A]))


def validate_mode(value):
    if not (value[CONF_INPUT] or value[CONF_OUTPUT]):
        raise cv.Invalid("Mode must be either input or output")
    if value[CONF_INPUT] and value[CONF_OUTPUT]:
        raise cv.Invalid("Mode must be either input or output")
    return value


TCA6408A_PIN_SCHEMA = cv.All(
    {
        cv.GenerateID(): cv.declare_id(TCA6408AGPIOPin),
        cv.Required(CONF_TCA6408A): cv.use_id(TCA6408AComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=0, max=17),
        cv.Optional(CONF_MODE, default={}): cv.All(
            {
                cv.Optional(CONF_INPUT, default=False): cv.boolean,
                cv.Optional(CONF_OUTPUT, default=False): cv.boolean,
            },
            validate_mode,
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
        cv.Optional(CONF_DEFAULT_ON, default=False): cv.boolean,
    }
)


@pins.PIN_SCHEMA_REGISTRY.register("tca6408a", TCA6408A_PIN_SCHEMA)
async def tca6408a_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_TCA6408A])

    cg.add(var.set_parent(parent))

    num = config[CONF_NUMBER]
    cg.add(var.set_pin(num))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    cg.add(var.set_state(config[CONF_DEFAULT_ON]))
    return var

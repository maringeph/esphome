"""DS100 3-Phase Energy Meter Component.

Supports Modbus RTU communication with DS100 series energy meters.

Features:
- Livedata: Instantaneous voltage, current, power, frequency per phase
- Statistics: Total and per-phase energy counters (active, reactive, import/export)
- Tariffs: Energy tracking across 4 configurable tariff periods
- Demand: Current power demand monitoring (per phase and total)
- Maximum Demand: Peak power demand tracking with reset capability
- Resettable Statistics: Separate resettable energy counters
- Device Configuration: Modbus settings (baud rate, parity, stop bits, address, etc.)

Platforms:
- sensor: Energy and power measurements
- select: Baud rate, parity, stop bits configuration
- number: Modbus address, scrolling time, demand period, password
- button: Reset maximum demand and resettable statistics
"""
import esphome.codegen as cg

AUTO_LOAD = ["modbus"]
CODEOWNERS = ["@maringeph"]

CONF_DS100_METER_ID = "ds100_meter_id"
ds100_meter_ns = cg.esphome_ns.namespace("ds100_meter")

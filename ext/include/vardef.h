#ifndef __VARDEF_H
#define __VARDEF_H

/*----------------------------------------------------------------------------
 * Firmware Structure Declarations
 *----------------------------------------------------------------------------*/

#pragma pack(1)
/* Node configuration
- v1:
- v2: Mega328PB => CSD, Up to 8 X lines, up to 32 Y lines
- v3: Tiny817 => 8PTC pins (Selectable X or Y), Driven shield
*/
typedef struct {
	uint8_t node_xmask;        /* Selects the X Pins for this node */
	uint8_t node_ymask;        /* Selects the Y Pins for this node */
	uint8_t node_csd;          /* Charge Share Delay */
	uint8_t node_rsel_prsc;    /* Bits 7:4 = Resistor, Bits 3:0  Prescaler */
	uint8_t node_gain;         /* Bits 7:4 = Analog gain, Bits 3:0 = Digital gain */
	uint8_t node_oversampling; /* Accumulator setting */
} qtm_acq_node_config_t;

typedef struct {
	uint16_t node_xmask;        /* Selects the X Pins for this node */
	uint16_t node_ymask;        /* Selects the Y Pins for this node */
	uint8_t  node_csd;          /* Charge Share Delay */
	uint8_t  node_rsel_prsc;    /* Bits 7:4 = Resistor, Bits 3:0  Prescaler */
	uint8_t  node_gain;         /* Bits 7:4 = Analog gain, Bits 3:0 = Digital gain */
	uint8_t  node_oversampling; /* Accumulator setting */
} qtm_acq_t161x_node_config_t;

typedef union {
	qtm_acq_node_config_t n8;
	qtm_acq_t161x_node_config_t n16;
} qtm_acq_union_node_config_t;

/* Node run-time data - Defined in common api as it will be used with all acquisition modules */

/* Node group configuration */
typedef struct qtm_acq_node_group_config_type {
	uint16_t num_sensor_nodes;    /* Number of sensor nodes */
	uint8_t  acq_sensor_type;     /* Self or mutual sensors */
	uint8_t  calib_option_select; /* Hardware tuning: XX | TT 3/4/5 Tau | X | XX None/RSEL/PRSC/CSD */
	uint8_t  freq_option_select;  /* SDS or ASDV setting */
} qtm_acq_node_group_config_t;

/* ---------------------------------------------------------------------------------------- */
/* Acquisition Node run-time data */
/* ---------------------------------------------------------------------------------------- */

typedef struct {
	uint8_t  node_acq_status;
	uint16_t node_acq_signals;
	uint16_t node_comp_caps;
} qtm_acq_node_data_t;

/* ---------------------------------------------------------------------------------------- */
/* Key sensor run-time data */
/* ---------------------------------------------------------------------------------------- */
typedef struct {
    uint8_t sensor_state;         /* Disabled, Off, On, Filter, Cal... */
    uint8_t sensor_state_counter; /* State counter */
    uint8_t node_data_struct_ptr[2]; /* Pointer to node data structure */
    uint16_t channel_reference;    /* Reference signal */
} qtm_touch_key_data_t;

#pragma pack()

/*-----------------------------------------------------------------------------*/
// Firmware Structure Declarations end
/*----------------------------------------------------------------------------*/

/* Extract Analog / Digital Gain */
#define NODE_GAIN_ANA(m) (uint8_t)(((m)&0xF0u) >> 4u)
#define NODE_GAIN_DIG(m) (uint8_t)((m)&0x0Fu)

#define NODE_BASE_LINE 512

#endif /* __VARDEF_H */
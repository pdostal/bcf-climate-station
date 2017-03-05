#ifndef _APPLICATION_H
#define _APPLICATION_H

#include <bcl.h>

// Forward declarations

void climate_event_event_handler(bc_module_climate_event_t event, void *event_param);
void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);

#endif // _APPLICATION_H

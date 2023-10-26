// This module is responsible for operating the LTE Modem Hardware. This includes power management and cell updates.
// It generates events 

#define await(k);

int location_handler() {

    while (1) {
        Location location = await fetch_location();

        { location, sensor, gnss_correct } = await {fetch_location(), fetch_gyroscope(), fetch_gnss_correction(); };

        calculated_location = fix_location(location, sensor, gnss_correct);

        server.push_location(calculated_location);
        
    } 
}



int capacity_handler() {    
    while (1) {
        button_state = ON_BUTTON_PRESS(btn1, btn2, btn3);


    }
}

int b() {

    Location_Generator()
}

// Everything coordinated from main/root program vs. a bunch of components throwing events all around the place


// Buffer?

// Continuously push events into a buffer, then we pull sometimes.

int r() {
    start_location_service();
    start_gyroscope_service();

}

REGISTER_COMPONENT(location_service, );

// How long 
consume_gyroscope_events(gyroscope_event) {
    sensor_processor_update(gyroscope_event);
}


#if defined(USE_SD_CARD_ON_SEND_FAIL)

// 
int subscriber_id = request_location_events(subscriber_id, HIGH_FREQ);

on_event(location_event) {
    process_machine_update(location_event);

    request_location_events(subscriber_id, LOW_FREQ);

    int priority = 1;
    bool resend_on_fail = false;

    send_location(priority,resend_on_fail); // DOn't need to await, this part of the code doesn't care whether this was successful or not.
};

// But, who gets to decide at what rate the gyroscope / location poll?
// Who controls that? Anyone?

// We need a sort of monolithic controller, that assumes responsibility for this.


// What if a different thread also wants location updates?

REGISTER_SERVICE(LOCATION_SERVICE_PROVIDER, );


// Services, which contain only Business Logic. Anything that can be siloed into its own thread without requiring too much intra-thread communication
// can be a service.

// Modules, which bridge the gap between drivers and BL. Must be able to serve multiple services if necessary. They control their own inner behavior without
// being directed by the service. For example, if no devices are requesting location, then the location module should shut down to save power. This can be KConfiged.

// Modules should not communicate with eachother, should use "third-party" to negotiate hardware use.
// Let's say 

// Modules try to find all available drivers to fulfill their purpose, then select one
// and provide a good interface to fulfill their functionality (without business logic).

// Modules can also interface with external library. E.g. Crypto Module.

// e.g: Cloud Module, GPS Module, 
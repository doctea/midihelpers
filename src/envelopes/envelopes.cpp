#include "envelopes/envelopes.h"

#ifdef ENABLE_SCREEN
    #include "mymenu.h"
    #include "menuitems_object.h"
    #include "submenuitem_bar.h"
    #include "mymenu/menuitems_envelopegraph.h"

    void EnvelopeBase::make_menu_items(Menu *menu, int index) {
        //#ifdef ENABLE_ENVELOPE_MENUS
            char label[40];
            snprintf(label, 40, "Envelope %i: %s", index, this->label);
            menu->add_page(label);

            SubMenuItemColumns *sub_menu_item_columns = new SubMenuItemColumns("Options", 3);

            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Attack",  this, &EnvelopeBase::set_attack,  &EnvelopeBase::get_attack,    nullptr, 0, 127, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Hold",    this, &EnvelopeBase::set_hold,    &EnvelopeBase::get_hold,      nullptr, 0, 127, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Decay",   this, &EnvelopeBase::set_decay,   &EnvelopeBase::get_decay,     nullptr, 0, 127, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Sustain", this, &EnvelopeBase::set_sustain, &EnvelopeBase::get_sustain,   nullptr, 0, 127, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Release", this, &EnvelopeBase::set_release, &EnvelopeBase::get_release,   nullptr, 0, 127, true, true));

            SubMenuItemBar *typebar = new SubMenuItemBar("Type");
            typebar->add(new ObjectToggleControl<EnvelopeBase>     ("Inverted",  this, &EnvelopeBase::set_invert,  &EnvelopeBase::is_invert,     nullptr));
            typebar->add(new ObjectToggleControl<EnvelopeBase>     ("Looping",   this, &EnvelopeBase::set_loop,    &EnvelopeBase::is_loop,       nullptr));

            //SubMenuItemBar *mod = new SubMenuItemBar("Modulation");
            typebar->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Mod HD",  this, &EnvelopeBase::set_mod_hd,  &EnvelopeBase::get_mod_hd,    nullptr, 0, 127, true, true));
            typebar->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Mod SR",  this, &EnvelopeBase::set_mod_sr,  &EnvelopeBase::get_mod_sr,    nullptr, 0, 127, true, true));

            typebar->add(new ObjectNumberControl<EnvelopeBase,uint8_t>("Sync", this, &EnvelopeBase::set_cc_value_sync_modifier, &EnvelopeBase::get_cc_value_sync_modifier, nullptr, 1, 24, true, true));

            menu->add(new EnvelopeDisplay("Graph", this));
            menu->add(new EnvelopeIndicator("Indicator", this));

            menu->add(sub_menu_item_columns);

            menu->add(typebar);
            //menu->add(mod);
        //#endif
    }
#endif
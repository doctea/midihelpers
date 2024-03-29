#include "envelopes/envelopes.h"

#ifdef ENABLE_SCREEN
    #include "mymenu.h"
    #include "menuitems_object.h"
    #include "submenuitem_bar.h"
    #include "mymenu/menuitems_envelopegraph.h"


    void EnvelopeBase::make_menu_items(Menu *menu, int index) {
        char label[40];
        snprintf(label, 40, "Envelope %i: %s", index, this->label);
        menu->add_page(label, C_WHITE, false);

        menu->add(new EnvelopeDisplay("Graph", this));
        menu->add(new EnvelopeIndicator("Indicator", this));
    }

    void RegularEnvelope::make_menu_items(Menu *menu, int index) {
        EnvelopeBase::make_menu_items(menu, index);
        //#ifdef ENABLE_ENVELOPE_MENUS
            menu->add(new EnvelopeDisplay("Graph", this));
            menu->add(new EnvelopeIndicator("Indicator", this));

            SubMenuItemColumns *sub_menu_item_columns = new SubMenuItemColumns("Options", 5);
            sub_menu_item_columns->show_header = false;

            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Att", this, &EnvelopeBase::set_attack,  &EnvelopeBase::get_attack,    nullptr, 0, 127, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Hld", this, &EnvelopeBase::set_hold,    &EnvelopeBase::get_hold,      nullptr, 0, 127, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Dec", this, &EnvelopeBase::set_decay,   &EnvelopeBase::get_decay,     nullptr, 0, 127, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Sus", this, &EnvelopeBase::set_sustain, &EnvelopeBase::get_sustain,   nullptr, 0, 127, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Rel", this, &EnvelopeBase::set_release, &EnvelopeBase::get_release,   nullptr, 0, 127, true, true));

            menu->add(sub_menu_item_columns);

            SubMenuItemBar *typebar = new SubMenuItemBar("Type");
            typebar->show_header = false;
            typebar->add(new ObjectToggleControl<EnvelopeBase>     ("Inverted",  this, &EnvelopeBase::set_invert,  &EnvelopeBase::is_invert,     nullptr));
            typebar->add(new ObjectToggleControl<EnvelopeBase>     ("Looping",   this, &EnvelopeBase::set_loop,    &EnvelopeBase::is_loop,       nullptr));

            //SubMenuItemBar *mod = new SubMenuItemBar("Modulation");
            typebar->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Mod HD",  this, &EnvelopeBase::set_mod_hd,  &EnvelopeBase::get_mod_hd,    nullptr, 0, 127, true, true));
            typebar->add(new ObjectNumberControl<EnvelopeBase,int8_t>("Mod SR",  this, &EnvelopeBase::set_mod_sr,  &EnvelopeBase::get_mod_sr,    nullptr, 0, 127, true, true));

            typebar->add(new ObjectNumberControl<EnvelopeBase,uint8_t>("Sync", this, &EnvelopeBase::set_cc_value_sync_modifier, &EnvelopeBase::get_cc_value_sync_modifier, nullptr, 1, 24, true, true));

            menu->add(typebar);
            //menu->add(mod);
        //#endif
    }
#endif
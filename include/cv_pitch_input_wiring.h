#pragma once

#include "parameter_inputs/ParameterInput.h"
#include "ParameterManager.h"
extern ParameterManager *parameter_manager;

#ifdef ENABLE_SCREEN
    #include "submenuitem_bar.h"
    #include "mymenu_items/ParameterInputMenuItems.h"
#endif

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
    #include "functional-vlpp.h"
#endif

namespace CVPitchInputWiring {

#ifdef ENABLE_SCREEN
    template<class TargetClass>
    struct SelectorPair {
        ParameterInputSelectorControl<TargetClass> *pitch = nullptr;
        ParameterInputSelectorControl<TargetClass> *velocity = nullptr;
    };

    template<class TargetClass>
    SelectorPair<TargetClass> add_parameter_input_selectors(
        SubMenuItemBar *bar,
        TargetClass *target,
        void(TargetClass::*set_pitch)(BaseParameterInput *),
        BaseParameterInput *(TargetClass::*get_pitch)(),
        void(TargetClass::*set_velocity)(BaseParameterInput *),
        BaseParameterInput *(TargetClass::*get_velocity)(),
        const char *pitch_label,
        const char *velocity_label
    ) {
        SelectorPair<TargetClass> selectors;

        selectors.pitch = new ParameterInputSelectorControl<TargetClass>(
            pitch_label,
            target,
            set_pitch,
            get_pitch,
            parameter_manager->get_available_pitch_inputs(),
            (target->*get_pitch)()
        );
        bar->add(selectors.pitch);

        selectors.velocity = new ParameterInputSelectorControl<TargetClass>(
            velocity_label,
            target,
            set_velocity,
            get_velocity,
            parameter_manager->available_inputs,
            (target->*get_velocity)()
        );
        bar->add(selectors.velocity);

        return selectors;
    }
#endif

#ifdef ENABLE_STORAGE
    template<class TargetClass>
    LSaveableSetting<const char *> *make_group_and_name_parameter_input_setting(
        const char *setting_name,
        const char *setting_group,
        TargetClass *target,
        void(TargetClass::*setter)(BaseParameterInput *),
        BaseParameterInput *(TargetClass::*getter)(),
        const char *unset_value,
        vl::Func<void(const char *)> on_missing = [](const char *) -> void {}
    ) {
        return new LSaveableSetting<const char *>(
            setting_name,
            setting_group,
            nullptr,
            [=](const char *value) {
                BaseParameterInput *source = (value && value[0]) ? parameter_manager->getInputForGroupAndName(value) : nullptr;
                if (source != nullptr || !(value && value[0])) {
                    (target->*setter)(source);
                } else {
                    (target->*setter)(nullptr);
                    on_missing(value);
                }
            },
            [=]() -> const char * {
                BaseParameterInput *source = (target->*getter)();
                if (source == nullptr) {
                    return unset_value;
                }
                return source->get_group_and_name();
            }
        );
    }
#endif

} // namespace CVPitchInputWiring

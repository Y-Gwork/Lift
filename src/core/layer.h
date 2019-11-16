﻿#pragma once

#include "core.h"
#include "events/event.h"

namespace lift {

class Layer {
public:
    Layer(std::string name = "Layer");
    virtual ~Layer();

    virtual void onAttach();
    virtual void onDetach();
    virtual void onUpdate();
    virtual void onImguiRender();
    virtual void onEvent(Event& event);

    [[nodiscard]] inline auto getName() const -> const std::string&;

protected:
    std::string name_;
};
}

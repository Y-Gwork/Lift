﻿#pragma once

#include "Core.h"
#include "Layer.h"

namespace lift {

	class LayerStack {
	public:
		LayerStack();
		~LayerStack();


		template <typename T>
		void PushLayer() {
			std::unique_ptr<T> layer = std::make_unique<T>();
			layer->OnAttach();
			layers_.emplace(layers_.begin() + layer_insert_index_++, std::move(layer));
		}

		template <typename T>
		void PushOverlay() {
			std::unique_ptr<T> layer = std::make_unique<T>();
			layer->OnAttach();
			layers_.emplace_back(std::move(layer));
		}

		template <typename T>
		void PopLayer(std::string name);
		template <typename T>
		void PopOverlay(std::string name);

		std::vector<std::unique_ptr<Layer>>::iterator begin();
		std::vector<std::unique_ptr<Layer>>::iterator end();
	private:
		std::vector<std::unique_ptr<Layer>> layers_;
		unsigned int layer_insert_index_ = 0;
	};
}
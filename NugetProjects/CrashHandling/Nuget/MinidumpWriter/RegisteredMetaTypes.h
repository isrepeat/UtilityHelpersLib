#pragma once
#include <QMetatype>
#include <functional>

// TODO: try to move in .cpp
namespace R {
	/* ------------------ */
	/*      Callbacks     */
	/* ------------------ */
	using Callback = std::function<void()>;
	Q_DECLARE_METATYPE(R::Callback);
	static const bool regCallback = qRegisterMetaType<Callback>();
}
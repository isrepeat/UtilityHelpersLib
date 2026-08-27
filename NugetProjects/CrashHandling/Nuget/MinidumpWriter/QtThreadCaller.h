#pragma once
#include "RegisteredMetaTypes.h"
#include <QThread>

class QtThreadCaller : public QObject {
	Q_OBJECT;
	QtThreadCaller();
public:
	~QtThreadCaller() = default;
	
	static QtThreadCaller& GetInstance();
	static void Invoke(R::Callback callback);

signals:
	void _InvokeInternal(R::Callback callback); // private signal
};
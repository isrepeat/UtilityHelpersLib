# Application framework

Каркас общий для Android host и PreviewPlugin. ApplicationSession владеет хранилищем состояния, репозиторием и PageManager. Хост передаёт размер поверхности, ввод и render backend; Kotlin не создаёт страницы и не хранит историю навигации.

## Навигация

- PageManager владеет PageRegistry, lifecycle страниц, input/animation controllers и историей. Зарегистрированные страницы и их bindings сохраняются до уничтожения сессии.
- Маршруты объявляются в PageManager::Routes(). Для перехода используются Trigger и типизированный NavigationState<T>.
- Main передаёт GreetingNavigationState.Message из ApplicationRepository. Settings требует этот payload и публикует Message в XAML binding.
- Перед активацией вызываются OnNavigatingFrom и OnNavigatingTo; контракт проверяет обязательность данных и TypeId. Отклонённый переход не меняет текущую страницу и историю.
- previousPage берётся из фактической истории. Загрузка уже активной страницы не сбрасывает стек.
- preview_Routes() публикует граф. Previewer передаёт в xp_navigate только transitionIds, а native создаёт и проверяет previewDefault. Весь путь проверяется до начала переходов; lifecycle может отклонить отдельный шаг.
- Reload разметки сохраняет экземпляр view model, payload и историю. Runtime bindings используют те же команды и свойства.

## Состояние и хост

ApplicationStateStore принимает callback сохранения документа. При ошибке сохранения прежний документ остаётся активным. В preview доступен отдельный документ в памяти и явный экспорт. AppRepositoryBase не зависит от предметной области. AppSessionController передаёт платформенные события через callback.

Android использует GLSurfaceView и JNI. ApplicationSession сохраняется при pause/resume; OpenGL renderer освобождается на GL-потоке. При уничтожении Activity сессия уничтожается. Постоянное хранение включается передачей callback в ApplicationSession.
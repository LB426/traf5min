Программа для загрузки трафика снятого на системе
FreeBSD при помощи NetGraph.

FreeBSD установлена на компе с двумя сетевыми картами работающими в режиме прозрачного моста (bridge).

Эта программа использовалась в системе ограничения трафика.
Задача системы - подсчитывать трафик для каждого IP адреса в локальной сети,
записывать метаинформацию о каждом IP пакете в БД (всё кроме самих данных)
и при привышении IP источником входящего трафика на определённую величину
генерировать команду IPFW для закрытия прохода через мост.


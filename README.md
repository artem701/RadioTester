<p>
lib/ - nrf SDK<br>
common/ - общая библиотека оберток над драйверами Nordic, использующихся в проекте<br>
Transmitter/ - папка проекта передатчика<br>
Receiver/ - папка проекта приемника<br>
Doc/ - графические описания протокола и схемы тестирования, описание CLI<br>
</p>
<br>
<p>
Программа позволяет проводить анализ качества передачи данных по радио в режиме IEEE 802.15.4 250Kbit,<br>
поддерживается настройка длины пакетов, паттерна заполнения, канала и мощности передачи, задержки между пакетами.<br>
</p>
<p>
Тестирование может происходить для конкретных значений канала и мощности, в таком случае выводится статистика<br>
по серии пакетов; может тестироваться конкретный канал, в таком случае определяется максимальное значение мощности,<br>
на которой происходит потеря данных, уровень шума на канале, а так же результат неудачного тестирования;<br>
может тестироваться интервал каналов или сразу все доступные каналы.<br>
</p>
<p>
Протокол передачи по SPI реализует нумерацию пакетов и простую контрольную сумму,<br>
что позволяет достаточно качественно отслеживать ошибки при передаче по SPI.<br>
</p>
<p>
CLI интерфейс передатчика реализуется посредством стандартной библиотеки nrf через UART соединение.<br>
</p>

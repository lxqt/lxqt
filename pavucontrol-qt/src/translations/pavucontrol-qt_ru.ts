<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru">
<context>
    <name>CardWidget</name>
    <message>
        <location filename="../cardwidget.ui" line="14"/>
        <source>Form</source>
        <translation>Форма</translation>
    </message>
    <message>
        <location filename="../cardwidget.ui" line="29"/>
        <source>Card Name</source>
        <translation>Имя карты</translation>
    </message>
    <message>
        <location filename="../cardwidget.ui" line="47"/>
        <source>Profile:</source>
        <translation>Профиль:</translation>
    </message>
    <message>
        <location filename="../cardwidget.cc" line="67"/>
        <source>pa_context_set_card_profile_by_index() failed</source>
        <translation>Сбой pa_context_set_card_profile_by_index()</translation>
    </message>
</context>
<context>
    <name>Channel</name>
    <message>
        <location filename="../channel.cc" line="89"/>
        <source>%1% (%2dB)</source>
        <comment>volume slider label [X% (YdB)]</comment>
        <translation>%1% (%2дБ)</translation>
    </message>
    <message>
        <location filename="../channel.cc" line="93"/>
        <source>%1%</source>
        <comment>volume slider label [X%]</comment>
        <translation>%1%</translation>
    </message>
    <message>
        <location filename="../channel.cc" line="160"/>
        <source>&lt;small&gt;Silence&lt;/small&gt;</source>
        <translation>&lt;small&gt;Тишина&lt;/small&gt;</translation>
    </message>
    <message>
        <location filename="../channel.cc" line="160"/>
        <source>&lt;small&gt;Min&lt;/small&gt;</source>
        <translation>&lt;small&gt;Минимум&lt;/small&gt;</translation>
    </message>
    <message>
        <location filename="../channel.cc" line="162"/>
        <source>&lt;small&gt;100% (0dB)&lt;/small&gt;</source>
        <translation>&lt;small&gt;100% (0дБ)&lt;/small&gt;</translation>
    </message>
    <message>
        <location filename="../channel.cc" line="165"/>
        <source>&lt;small&gt;&lt;i&gt;Base&lt;/i&gt;&lt;/small&gt;</source>
        <translation>&lt;small&gt;&lt;i&gt;База&lt;/i&gt;&lt;/small&gt;</translation>
    </message>
</context>
<context>
    <name>ChannelWidget</name>
    <message>
        <location filename="../channelwidget.ui" line="14"/>
        <source>Form</source>
        <translation>Форма</translation>
    </message>
    <message>
        <location filename="../channelwidget.ui" line="20"/>
        <source>&lt;b&gt;left-front&lt;/b&gt;</source>
        <translation>&lt;b&gt;левый-передний&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../channelwidget.ui" line="34"/>
        <source>&lt;small&gt;50%&lt;/small&gt;</source>
        <translation>&lt;small&gt;50%&lt;/small&gt;</translation>
    </message>
</context>
<context>
    <name>DeviceWidget</name>
    <message>
        <location filename="../devicewidget.ui" line="14"/>
        <source>Form</source>
        <translation>Форма</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="36"/>
        <source>Device Title</source>
        <translation>Имя устройства</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="56"/>
        <source>Mute audio</source>
        <translation>Выключить звук</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="69"/>
        <source>Lock channels together</source>
        <translation>Связать каналы вместе</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="85"/>
        <source>Set as fallback</source>
        <translation>Установить как резервное</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="103"/>
        <source>&lt;b&gt;Port:&lt;/b&gt;</source>
        <translation>&lt;b&gt;Профиль:&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="126"/>
        <source>Show advanced options</source>
        <translation>Показать расширенные настройки</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="172"/>
        <source>PCM</source>
        <translation>PCM</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="182"/>
        <source>AC3</source>
        <translation>AC3</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="189"/>
        <source>EAC3</source>
        <translation>EAC3</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="196"/>
        <source>DTS</source>
        <translation>DTS</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="203"/>
        <source>MPEG</source>
        <translation>MPEG</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="210"/>
        <source>AAC</source>
        <translation>AAC</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="238"/>
        <source>&lt;b&gt;Latency offset:&lt;/b&gt;</source>
        <translation>&lt;b&gt;Время задержки:&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="245"/>
        <source> ms</source>
        <translation> мс</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="41"/>
        <source>Rename device...</source>
        <translation>Переименовать устройство...</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="155"/>
        <source>pa_context_set_port_latency_offset() failed</source>
        <translation>Сбой pa_context_set_port_latency_offset()</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="226"/>
        <source>Sorry, but device renaming is not supported.</source>
        <translation>Простите, но переименование устройств не поддерживается.</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="227"/>
        <source>You need to load module-device-manager in the PulseAudio server in order to rename devices</source>
        <translation>Чтобы переименовывать устройства, вам необходимо загрузить module-device-manager в сервер PulseAudio</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="233"/>
        <source>Rename device %1 to:</source>
        <translation>Переименовать устройство %1 в:</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="240"/>
        <source>pa_ext_device_manager_set_device_description() failed</source>
        <translation>Сбой pa_ext_device_manager_set_device_description()</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../mainwindow.ui" line="14"/>
        <source>Volume Control</source>
        <translation>Громкость</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="28"/>
        <source>&amp;Playback</source>
        <translation>&amp;Проигрывание</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="55"/>
        <source>&lt;i&gt;No application is currently playing audio.&lt;/i&gt;</source>
        <translation>&lt;i&gt;Ни одно приложение сейчас не воспроизводит звук.&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="66"/>
        <location filename="../mainwindow.ui" line="131"/>
        <location filename="../mainwindow.ui" line="196"/>
        <location filename="../mainwindow.ui" line="261"/>
        <source>Show:</source>
        <translation>Показать:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="74"/>
        <location filename="../mainwindow.ui" line="139"/>
        <source>All Streams</source>
        <translation>Все потоки</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="79"/>
        <location filename="../mainwindow.ui" line="144"/>
        <source>Applications</source>
        <translation>Приложения</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="84"/>
        <location filename="../mainwindow.ui" line="149"/>
        <source>Virtual Streams</source>
        <translation>Виртуальные потоки</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="93"/>
        <source>&amp;Recording</source>
        <translation>&amp;Запись</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="120"/>
        <source>&lt;i&gt;No application is currently recording audio.&lt;/i&gt;</source>
        <translation>&lt;i&gt;Ни одно приложение сейчас не записывает звук.&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="158"/>
        <source>&amp;Output Devices</source>
        <translation>Устройства В&amp;ывода</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="185"/>
        <source>&lt;i&gt;No output devices available&lt;/i&gt;</source>
        <translation>&lt;i&gt;Нет доступных устройств вывода.&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="204"/>
        <source>All Output Devices</source>
        <translation>Все устройства вывода</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="209"/>
        <source>Hardware Output Devices</source>
        <translation>Аппаратные устройства вывода</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="214"/>
        <source>Virtual Output Devices</source>
        <translation>Виртуальные устройства вывода</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="223"/>
        <source>&amp;Input Devices</source>
        <translation>Устройства &amp;Ввода</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="250"/>
        <source>&lt;i&gt;No input devices available&lt;/i&gt;</source>
        <translation>&lt;i&gt;Нет доступных устройств ввода.&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="269"/>
        <source>All Input Devices</source>
        <translation>Все устройства ввода</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="274"/>
        <source>All Except Monitors</source>
        <translation>Все кроме мониторов</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="279"/>
        <source>Hardware Input Devices</source>
        <translation>Аппаратные устройства ввода</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="284"/>
        <source>Virtual Input Devices</source>
        <translation>Виртуальные устройства ввода</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="289"/>
        <source>Monitors</source>
        <translation>Мониторы</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="298"/>
        <source>&amp;Configuration</source>
        <translation>&amp;Конфигурация</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="325"/>
        <source>&lt;i&gt;No cards available for configuration&lt;/i&gt;</source>
        <translation>&lt;i&gt;Нет доступных для конфигурации карт.&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="336"/>
        <source>Show volume meters</source>
        <translation>Показывать измерители громкости</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="347"/>
        <source>...</source>
        <translation>...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="159"/>
        <source> (plugged in)</source>
        <translation> (подключено)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="163"/>
        <location filename="../mainwindow.cc" line="257"/>
        <source> (unavailable)</source>
        <translation> (недоступно)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="165"/>
        <location filename="../mainwindow.cc" line="254"/>
        <source> (unplugged)</source>
        <translation> (отключено)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="388"/>
        <source>Failed to read data from stream</source>
        <translation>Не удается прочесть данные из потока</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="432"/>
        <source>Peak detect</source>
        <translation>Детектирование пиков</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="433"/>
        <source>Failed to create monitoring stream</source>
        <translation>Не удается создать поток мониторинга</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="448"/>
        <source>Failed to connect monitoring stream</source>
        <translation>Не удается присоединить поток мониторинга</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="587"/>
        <source>Ignoring sink-input due to it being designated as an event and thus handled by the Event widget</source>
        <translation>Входные данные приёмника игнорируются, так как они рассматриваются как событие и будут обработаны виджетом событий</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="759"/>
        <source>System Sounds</source>
        <translation>Системные звуки</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="1089"/>
        <source>Establishing connection to PulseAudio. Please wait...</source>
        <translation>Установление соединения с PulseAudio. Подождите...</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../pavucontrol.cc" line="66"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="87"/>
        <source>Card callback failure</source>
        <translation>Сбой обратного вызова карты</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="110"/>
        <source>Sink callback failure</source>
        <translation>Сбой обратного вызова слива</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="133"/>
        <source>Source callback failure</source>
        <translation>Сбой обратного вызова источника</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="152"/>
        <source>Sink input callback failure</source>
        <translation>Сбой обратного вызова входа слива</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="171"/>
        <source>Source output callback failure</source>
        <translation>Сбой обратного вызова выхода источника</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="211"/>
        <source>Client callback failure</source>
        <translation>Сбой обратного вызова клиента</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="227"/>
        <source>Server info callback failure</source>
        <translation>Сбой обратного вызова информации о сервере</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="245"/>
        <location filename="../pavucontrol.cc" line="542"/>
        <source>Failed to initialize stream_restore extension: %s</source>
        <translation>Сбой инициализации расширения stream_restore: %s</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="263"/>
        <source>pa_ext_stream_restore_read() failed</source>
        <translation>Сбой pa_ext_stream_restore_read()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="281"/>
        <location filename="../pavucontrol.cc" line="556"/>
        <source>Failed to initialize device restore extension: %s</source>
        <translation>Сбой инициализации расширения восстановления устройства: %s</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="302"/>
        <source>pa_ext_device_restore_read_sink_formats() failed</source>
        <translation>Сбой pa_ext_device_restore_read_sink_formats()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="320"/>
        <location filename="../pavucontrol.cc" line="569"/>
        <source>Failed to initialize device manager extension: %s</source>
        <translation>Сбой инициализации расширения диспетчера устройств: %s</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="339"/>
        <source>pa_ext_device_manager_read() failed</source>
        <translation>Сбой pa_ext_device_manager_read()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="356"/>
        <source>pa_context_get_sink_info_by_index() failed</source>
        <translation>Сбой pa_context_get_sink_info_by_index()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="369"/>
        <source>pa_context_get_source_info_by_index() failed</source>
        <translation>Сбой pa_context_get_source_info_by_index()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="382"/>
        <location filename="../pavucontrol.cc" line="395"/>
        <source>pa_context_get_sink_input_info() failed</source>
        <translation>Сбой pa_context_get_sink_input_info()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="408"/>
        <source>pa_context_get_client_info() failed</source>
        <translation>Сбой pa_context_get_client_info()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="418"/>
        <location filename="../pavucontrol.cc" line="483"/>
        <source>pa_context_get_server_info() failed</source>
        <translation>Сбой pa_context_get_server_info()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="431"/>
        <source>pa_context_get_card_info_by_index() failed</source>
        <translation>Сбой pa_context_get_card_info_by_index()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="474"/>
        <source>pa_context_subscribe() failed</source>
        <translation>Сбой pa_context_subscribe()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="490"/>
        <source>pa_context_client_info_list() failed</source>
        <translation>Сбой pa_context_client_info_list()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="497"/>
        <source>pa_context_get_card_info_list() failed</source>
        <translation>Сбой pa_context_get_card_info_list()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="504"/>
        <source>pa_context_get_sink_info_list() failed</source>
        <translation>Сбой pa_context_get_sink_info_list()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="511"/>
        <source>pa_context_get_source_info_list() failed</source>
        <translation>Сбой pa_context_get_source_info_list()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="518"/>
        <source>pa_context_get_sink_input_info_list() failed</source>
        <translation>Сбой pa_context_get_sink_input_info_list()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="525"/>
        <source>pa_context_get_source_output_info_list() failed</source>
        <translation>Сбой pa_context_get_source_output_info_list()</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="584"/>
        <location filename="../pavucontrol.cc" line="635"/>
        <source>Connection failed, attempting reconnect</source>
        <translation>Ошибка подключения, пытаюсь переподключит</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="607"/>
        <location filename="../pavucontrol.cc" line="664"/>
        <source>PulseAudio Volume Control</source>
        <translation>Регулятор громкости PulseAudio</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="622"/>
        <source>Connection to PulseAudio failed. Automatic retry in 5s

In this case this is likely because PULSE_SERVER in the Environment/X11 Root Window Properties
or default-server in client.conf is misconfigured.
This situation can also arrise when PulseAudio crashed and left stale details in the X11 Root Window.
If this is the case, then PulseAudio should autospawn again, or if this is not configured you should
run start-pulseaudio-x11 manually.</source>
        <translation>Не удалось подключиться к PulseAudio. Повторная попытка через 5 с

В данном случае это вероятно вызвано неверным значением PULSE_SERVER в свойствах
корневого окна, в переменных окружения или в ключе default-server в client.conf.
Эта ситуация также может возникнуть, если PulseAudio аварийно завершилась и свойства
корневого окна X11 содержат устаревшую информацию.
В этом случае PulseAudio должна запуститься снова; если же система так не настроена,
вы должны запустить start-pulseaudio-x11 вручную.</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="672"/>
        <source>Select a specific tab on load.</source>
        <translation>Выбрать вкладку на загрузку.</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="675"/>
        <source>Retry forever if pa quits (every 5 seconds).</source>
        <translation>Повторять вечно, если pa завершит работу (каждые 5 секунд).</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="678"/>
        <source>Maximize the window.</source>
        <translation>Распахнуть окно.</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="703"/>
        <source>Fatal Error: Unable to connect to PulseAudio</source>
        <translation>Критическая ошибка: Невозможно подключиться к PulseAudio</translation>
    </message>
</context>
<context>
    <name>RoleWidget</name>
    <message>
        <location filename="../rolewidget.cc" line="59"/>
        <source>pa_ext_stream_restore_write() failed</source>
        <translation>Сбой pa_ext_stream_restore_write()</translation>
    </message>
</context>
<context>
    <name>SinkInputWidget</name>
    <message>
        <location filename="../sinkinputwidget.cc" line="36"/>
        <source>on</source>
        <translation>вкл.</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="39"/>
        <source>Terminate Playback</source>
        <translation>Прервать воспроизведение</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="53"/>
        <source>Unknown output</source>
        <translation>Неизвестный вывод</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="64"/>
        <source>pa_context_set_sink_input_volume() failed</source>
        <translation>Сбой pa_context_set_sink_input_volume()</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="79"/>
        <source>pa_context_set_sink_input_mute() failed</source>
        <translation>Сбой pa_context_set_sink_input_mute()</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="89"/>
        <source>pa_context_kill_sink_input() failed</source>
        <translation>Сбой pa_context_kill_sink_input()</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="114"/>
        <source>pa_context_move_sink_input_by_index() failed</source>
        <translation>Сбой pa_context_move_sink_input_by_index()</translation>
    </message>
</context>
<context>
    <name>SinkWidget</name>
    <message>
        <location filename="../sinkwidget.cc" line="81"/>
        <source>pa_context_set_sink_volume_by_index() failed</source>
        <translation>Сбой pa_context_set_sink_volume_by_index()</translation>
    </message>
    <message>
        <location filename="../sinkwidget.cc" line="96"/>
        <source>pa_context_set_sink_mute_by_index() failed</source>
        <translation>Сбой pa_context_set_sink_mute_by_index()</translation>
    </message>
    <message>
        <location filename="../sinkwidget.cc" line="110"/>
        <source>pa_context_set_default_sink() failed</source>
        <translation>Сбой pa_context_set_default_sink()</translation>
    </message>
    <message>
        <location filename="../sinkwidget.cc" line="126"/>
        <source>pa_context_set_sink_port_by_index() failed</source>
        <translation>Сбой pa_context_set_sink_mute_by_index()</translation>
    </message>
    <message>
        <location filename="../sinkwidget.cc" line="166"/>
        <source>pa_ext_device_restore_save_sink_formats() failed</source>
        <translation>Сбой pa_ext_device_restore_save_sink_formats()</translation>
    </message>
</context>
<context>
    <name>SourceOutputWidget</name>
    <message>
        <location filename="../sourceoutputwidget.cc" line="35"/>
        <source>from</source>
        <translation>из</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="39"/>
        <source>Terminate Recording</source>
        <translation>Прервать запись</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="60"/>
        <source>Unknown input</source>
        <translation>Неизвестный вывод</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="72"/>
        <source>pa_context_set_source_output_volume() failed</source>
        <translation>Сбой pa_context_set_source_output_volume()</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="87"/>
        <source>pa_context_set_source_output_mute() failed</source>
        <translation>Сбой pa_context_set_source_output_mute()</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="98"/>
        <source>pa_context_kill_source_output() failed</source>
        <translation>Сбой pa_context_kill_source_output()</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="125"/>
        <source>pa_context_move_source_output_by_index() failed</source>
        <translation>Сбой pa_context_move_source_output_by_index()</translation>
    </message>
</context>
<context>
    <name>SourceWidget</name>
    <message>
        <location filename="../sourcewidget.cc" line="35"/>
        <source>pa_context_set_source_volume_by_index() failed</source>
        <translation>Сбой pa_context_set_source_volume_by_index()</translation>
    </message>
    <message>
        <location filename="../sourcewidget.cc" line="50"/>
        <source>pa_context_set_source_mute_by_index() failed</source>
        <translation>Сбой pa_context_set_source_mute_by_index()</translation>
    </message>
    <message>
        <location filename="../sourcewidget.cc" line="64"/>
        <source>pa_context_set_default_source() failed</source>
        <translation>Сбой pa_context_set_default_source()</translation>
    </message>
    <message>
        <location filename="../sourcewidget.cc" line="80"/>
        <source>pa_context_set_source_port_by_index() failed</source>
        <translation>Сбой pa_context_set_source_mute_by_index()</translation>
    </message>
</context>
<context>
    <name>StreamWidget</name>
    <message>
        <location filename="../streamwidget.ui" line="14"/>
        <source>Form</source>
        <translation>Форма</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="32"/>
        <source>Device Title</source>
        <translation>Название устройства</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="52"/>
        <source>direction</source>
        <translation>направление</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="59"/>
        <source>device</source>
        <translation>устройство</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="66"/>
        <source>Mute audio</source>
        <translation>Выключить звук</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="79"/>
        <source>Lock channels together</source>
        <translation>Связать каналы вместе</translation>
    </message>
    <message>
        <location filename="../streamwidget.cc" line="34"/>
        <source>Terminate</source>
        <translation>Прервать поток</translation>
    </message>
</context>
</TS>

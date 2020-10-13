<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="nb_NO">
<context>
    <name>CardWidget</name>
    <message>
        <location filename="../cardwidget.ui" line="14"/>
        <source>Form</source>
        <translation>Formular</translation>
    </message>
    <message>
        <location filename="../cardwidget.ui" line="29"/>
        <source>Card Name</source>
        <translation>Kortnavn</translation>
    </message>
    <message>
        <location filename="../cardwidget.ui" line="47"/>
        <source>Profile:</source>
        <translation>Profil:</translation>
    </message>
    <message>
        <location filename="../cardwidget.cc" line="67"/>
        <source>pa_context_set_card_profile_by_index() failed</source>
        <translation>pa_context_set_card_profile_by_index() virket ikke</translation>
    </message>
</context>
<context>
    <name>Channel</name>
    <message>
        <location filename="../channel.cc" line="89"/>
        <source>%1% (%2dB)</source>
        <comment>volume slider label [X% (YdB)]</comment>
        <translation></translation>
    </message>
    <message>
        <location filename="../channel.cc" line="93"/>
        <source>%1%</source>
        <comment>volume slider label [X%]</comment>
        <translation></translation>
    </message>
    <message>
        <location filename="../channel.cc" line="160"/>
        <source>&lt;small&gt;Silence&lt;/small&gt;</source>
        <translation>&lt;small&gt;Stillhet&lt;/small&gt;</translation>
    </message>
    <message>
        <location filename="../channel.cc" line="160"/>
        <source>&lt;small&gt;Min&lt;/small&gt;</source>
        <translation>&lt;small&gt;Min.&lt;/small&gt;</translation>
    </message>
    <message>
        <location filename="../channel.cc" line="162"/>
        <source>&lt;small&gt;100% (0dB)&lt;/small&gt;</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../channel.cc" line="165"/>
        <source>&lt;small&gt;&lt;i&gt;Base&lt;/i&gt;&lt;/small&gt;</source>
        <translation>&lt;small&gt;&lt;i&gt;Basis&lt;/i&gt;&lt;/small&gt;</translation>
    </message>
</context>
<context>
    <name>ChannelWidget</name>
    <message>
        <location filename="../channelwidget.ui" line="14"/>
        <source>Form</source>
        <translation>Formular</translation>
    </message>
    <message>
        <location filename="../channelwidget.ui" line="20"/>
        <source>&lt;b&gt;left-front&lt;/b&gt;</source>
        <translation>&lt;b&gt;Venstre foran&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../channelwidget.ui" line="34"/>
        <source>&lt;small&gt;50%&lt;/small&gt;</source>
        <translation></translation>
    </message>
</context>
<context>
    <name>DeviceWidget</name>
    <message>
        <location filename="../devicewidget.ui" line="14"/>
        <source>Form</source>
        <translation>Formular</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="36"/>
        <source>Device Title</source>
        <translation>Enhetsnavn</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="56"/>
        <source>Mute audio</source>
        <translation>Skru av lyd</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="69"/>
        <source>Lock channels together</source>
        <translation>Lås kanaler sammen</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="85"/>
        <source>Set as fallback</source>
        <translation>Sett som reserve</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="103"/>
        <source>&lt;b&gt;Port:&lt;/b&gt;</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="126"/>
        <source>Show advanced options</source>
        <translation>Vis avanserte innstillinger</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="172"/>
        <source>PCM</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="182"/>
        <source>AC3</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="189"/>
        <source>EAC3</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="196"/>
        <source>DTS</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="203"/>
        <source>MPEG</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="210"/>
        <source>AAC</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="238"/>
        <source>&lt;b&gt;Latency offset:&lt;/b&gt;</source>
        <translation>&lt;b&gt;Forsinkelsesjustering:&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../devicewidget.ui" line="245"/>
        <source> ms</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="41"/>
        <source>Rename device...</source>
        <translation>Gi enheten nytt navn..</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="155"/>
        <source>pa_context_set_port_latency_offset() failed</source>
        <translation>pa_context_set_port_latency_offset() mislyktes</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="226"/>
        <source>Sorry, but device renaming is not supported.</source>
        <translation>Unnskyld, men å gi enheten nytt navn støttes ikke.</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="227"/>
        <source>You need to load module-device-manager in the PulseAudio server in order to rename devices</source>
        <translation>Du må laste inn module-device-manager i PulseAudio-serveren for å kunne gi nytt navn til enheter</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="233"/>
        <source>Rename device %1 to:</source>
        <translation>Gi enheten %1 nytt navn:</translation>
    </message>
    <message>
        <location filename="../devicewidget.cc" line="240"/>
        <source>pa_ext_device_manager_set_device_description() failed</source>
        <translation>pa_ext_device_manager_set_device_description() mislyktes</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../mainwindow.ui" line="14"/>
        <source>Volume Control</source>
        <translation>Volumkontroll</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="55"/>
        <source>&lt;i&gt;No application is currently playing audio.&lt;/i&gt;</source>
        <translation>&lt;i&gt;Ingen programmer spiller lyd nå.&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="66"/>
        <location filename="../mainwindow.ui" line="131"/>
        <location filename="../mainwindow.ui" line="196"/>
        <location filename="../mainwindow.ui" line="261"/>
        <source>Show:</source>
        <translation>Vis:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="74"/>
        <location filename="../mainwindow.ui" line="139"/>
        <source>All Streams</source>
        <translation>Alle strømmer</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="79"/>
        <location filename="../mainwindow.ui" line="144"/>
        <source>Applications</source>
        <translation>Programmer</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="84"/>
        <location filename="../mainwindow.ui" line="149"/>
        <source>Virtual Streams</source>
        <translation>Virtuelle strømmer</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="120"/>
        <source>&lt;i&gt;No application is currently recording audio.&lt;/i&gt;</source>
        <translation>&lt;i&gt;Ingen programmer tar opp lyd nå.&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="185"/>
        <source>&lt;i&gt;No output devices available&lt;/i&gt;</source>
        <translation>&lt;i&gt;Ingen utenheter er tilgjengelige&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="204"/>
        <source>All Output Devices</source>
        <translation>Alle utenheter</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="209"/>
        <source>Hardware Output Devices</source>
        <translation>Maskinvareutenheter</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="214"/>
        <source>Virtual Output Devices</source>
        <translation>Virtuelle utenheter</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="28"/>
        <source>&amp;Playback</source>
        <translation>&amp;Avspilling</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="93"/>
        <source>&amp;Recording</source>
        <translation>&amp;Opptak</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="158"/>
        <source>&amp;Output Devices</source>
        <translation>&amp;Utenheter</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="223"/>
        <source>&amp;Input Devices</source>
        <translation>&amp;Innenheter</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="250"/>
        <source>&lt;i&gt;No input devices available&lt;/i&gt;</source>
        <translation>&lt;i&gt;Ingen innenheter er tilgjengelige&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="269"/>
        <source>All Input Devices</source>
        <translation>Alle innenheter</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="274"/>
        <source>All Except Monitors</source>
        <translation>Alle untatt monitorer</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="279"/>
        <source>Hardware Input Devices</source>
        <translation>Maskinvare innenheter</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="284"/>
        <source>Virtual Input Devices</source>
        <translation>Virtuelle innenheter</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="289"/>
        <source>Monitors</source>
        <translation>Monitorer</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="298"/>
        <source>&amp;Configuration</source>
        <translation>&amp;Konfigurasjon</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="325"/>
        <source>&lt;i&gt;No cards available for configuration&lt;/i&gt;</source>
        <translation>&lt;i&gt;Ingen kort er tilgjengelige for innstilling&lt;/i&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="336"/>
        <source>Show volume meters</source>
        <translation>Vis volumindikatorer</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="347"/>
        <source>...</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="159"/>
        <source> (plugged in)</source>
        <translation> (tilkoblet)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="163"/>
        <location filename="../mainwindow.cc" line="257"/>
        <source> (unavailable)</source>
        <translation> (ikke tilgjengelig)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="165"/>
        <location filename="../mainwindow.cc" line="254"/>
        <source> (unplugged)</source>
        <translation> (frakoblet)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="388"/>
        <source>Failed to read data from stream</source>
        <translation>Klarte ikke å lese data fra strøm</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="432"/>
        <source>Peak detect</source>
        <translation>Overstyringsoppdaging</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="433"/>
        <source>Failed to create monitoring stream</source>
        <translation>Klarte ikke å lage monitorstrøm</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="448"/>
        <source>Failed to connect monitoring stream</source>
        <translation>Klarte ikke å koble til monitorstrøm</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="587"/>
        <source>Ignoring sink-input due to it being designated as an event and thus handled by the Event widget</source>
        <translation>Ignorerer sink-input fordi den er satt opp til å håndteres som en hendelse og dermed blir håndtert av Event widgeten</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="759"/>
        <source>System Sounds</source>
        <translation>Systemlyder</translation>
    </message>
    <message>
        <location filename="../mainwindow.cc" line="1089"/>
        <source>Establishing connection to PulseAudio. Please wait...</source>
        <translation>Setter opp forbindelse til PulseAudio. Vennligst vent...</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../pavucontrol.cc" line="66"/>
        <source>Error</source>
        <translation>Feil</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="87"/>
        <source>Card callback failure</source>
        <translation>Feil med tilbakemelding til kort</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="110"/>
        <source>Sink callback failure</source>
        <translation>Feil med tilbakemelding til sink</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="133"/>
        <source>Source callback failure</source>
        <translation>Feil med tilbakemelding til kilde</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="152"/>
        <source>Sink input callback failure</source>
        <translation>Feil med tilbakemelding til inngang på sink</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="171"/>
        <source>Source output callback failure</source>
        <translation>Feil med tilbakemelding til kildens utgang</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="211"/>
        <source>Client callback failure</source>
        <translation>Feil med tilbakemelding til klient</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="227"/>
        <source>Server info callback failure</source>
        <translation>Feil med tilbakemelding om server info</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="245"/>
        <location filename="../pavucontrol.cc" line="542"/>
        <source>Failed to initialize stream_restore extension: %s</source>
        <translation>Feil med å sette i verk stream_restore extension: %s</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="263"/>
        <source>pa_ext_stream_restore_read() failed</source>
        <translation>pa_ext_stream_restore_read() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="281"/>
        <location filename="../pavucontrol.cc" line="556"/>
        <source>Failed to initialize device restore extension: %s</source>
        <translation>Klarte ikke å sette i gang device restore extension: %s</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="302"/>
        <source>pa_ext_device_restore_read_sink_formats() failed</source>
        <translation>pa_ext_device_restore_read_sink_formats() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="320"/>
        <location filename="../pavucontrol.cc" line="569"/>
        <source>Failed to initialize device manager extension: %s</source>
        <translation>Feil med å sette i gang device manager extension: %s</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="339"/>
        <source>pa_ext_device_manager_read() failed</source>
        <translation>pa_ext_device_manager_read() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="356"/>
        <source>pa_context_get_sink_info_by_index() failed</source>
        <translation>pa_context_get_sink_info_by_index() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="369"/>
        <source>pa_context_get_source_info_by_index() failed</source>
        <translation>pa_context_get_source_info_by_index() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="382"/>
        <location filename="../pavucontrol.cc" line="395"/>
        <source>pa_context_get_sink_input_info() failed</source>
        <translation>pa_context_get_sink_input_info() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="408"/>
        <source>pa_context_get_client_info() failed</source>
        <translation>pa_context_get_client_info() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="418"/>
        <location filename="../pavucontrol.cc" line="483"/>
        <source>pa_context_get_server_info() failed</source>
        <translation>pa_context_get_server_info() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="431"/>
        <source>pa_context_get_card_info_by_index() failed</source>
        <translation>pa_context_get_card_info_by_index() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="474"/>
        <source>pa_context_subscribe() failed</source>
        <translation>pa_context_subscribe() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="490"/>
        <source>pa_context_client_info_list() failed</source>
        <translation>pa_context_client_info_list() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="497"/>
        <source>pa_context_get_card_info_list() failed</source>
        <translation>pa_context_get_card_info_list() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="504"/>
        <source>pa_context_get_sink_info_list() failed</source>
        <translation>pa_context_get_sink_info_list() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="511"/>
        <source>pa_context_get_source_info_list() failed</source>
        <translation>pa_context_get_source_info_list() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="518"/>
        <source>pa_context_get_sink_input_info_list() failed</source>
        <translation>pa_context_get_sink_input_info_list() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="525"/>
        <source>pa_context_get_source_output_info_list() failed</source>
        <translation>pa_context_get_source_output_info_list() mislyktes</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="584"/>
        <location filename="../pavucontrol.cc" line="635"/>
        <source>Connection failed, attempting reconnect</source>
        <translation>Forbindelsen virket ikke, forsøker å sette opp ny forbindelse</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="607"/>
        <location filename="../pavucontrol.cc" line="664"/>
        <source>PulseAudio Volume Control</source>
        <translation>PulseAudio volumkontroll</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="622"/>
        <source>Connection to PulseAudio failed. Automatic retry in 5s

In this case this is likely because PULSE_SERVER in the Environment/X11 Root Window Properties
or default-server in client.conf is misconfigured.
This situation can also arrise when PulseAudio crashed and left stale details in the X11 Root Window.
If this is the case, then PulseAudio should autospawn again, or if this is not configured you should
run start-pulseaudio-x11 manually.</source>
        <translation>Forbindelsen til PulseAudio mislyktes. Prøver igjen om 5 sekunder

Denne gangen er det sannsynligvis fordi PULSE_SERVER i skrivebordsmiljøet eller X11 Root Window Properties
eller i standard-serveren i client.conf er feilinnstilt.
Denne situasjonen kan også oppstå når PulseAudio har kræsjet og har etterlatt detaljer i X11 Root Window.
Hvis så er tilfelle burde PulseAudio autooppstarte igjen. Hvis dette ikke er stilt inn 
burde du kjøre start-pulseaudio-x11 manuelt.</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="672"/>
        <source>Select a specific tab on load.</source>
        <translation>Velg en spesifikk fane ved innlasting.</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="675"/>
        <source>Retry forever if pa quits (every 5 seconds).</source>
        <translation>Prøv igjen for alltid hvis pa avslutter (hvert femte sekund).</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="678"/>
        <source>Maximize the window.</source>
        <translation>Maksimer vinduet.</translation>
    </message>
    <message>
        <location filename="../pavucontrol.cc" line="703"/>
        <source>Fatal Error: Unable to connect to PulseAudio</source>
        <translation>Kritisk feil: Kunne ikke koble til PulseAudio</translation>
    </message>
</context>
<context>
    <name>RoleWidget</name>
    <message>
        <location filename="../rolewidget.cc" line="59"/>
        <source>pa_ext_stream_restore_write() failed</source>
        <translation>pa_ext_stream_restore_write() mislyktes</translation>
    </message>
</context>
<context>
    <name>SinkInputWidget</name>
    <message>
        <location filename="../sinkinputwidget.cc" line="36"/>
        <source>on</source>
        <translation>på</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="39"/>
        <source>Terminate Playback</source>
        <translation>Slå av avspilling</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="53"/>
        <source>Unknown output</source>
        <translation>Ukjent utgang</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="64"/>
        <source>pa_context_set_sink_input_volume() failed</source>
        <translation>pa_context_set_sink_input_volume() mislyktes</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="79"/>
        <source>pa_context_set_sink_input_mute() failed</source>
        <translation>pa_context_set_sink_input_mute() mislyktes</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="89"/>
        <source>pa_context_kill_sink_input() failed</source>
        <translation>pa_context_kill_sink_input() mislyktes</translation>
    </message>
    <message>
        <location filename="../sinkinputwidget.cc" line="114"/>
        <source>pa_context_move_sink_input_by_index() failed</source>
        <translation>pa_context_move_sink_input_by_index() mislyktes</translation>
    </message>
</context>
<context>
    <name>SinkWidget</name>
    <message>
        <location filename="../sinkwidget.cc" line="81"/>
        <source>pa_context_set_sink_volume_by_index() failed</source>
        <translation>pa_context_set_sink_volume_by_index() mislyktes</translation>
    </message>
    <message>
        <location filename="../sinkwidget.cc" line="96"/>
        <source>pa_context_set_sink_mute_by_index() failed</source>
        <translation>pa_context_set_sink_mute_by_index() mislyktes</translation>
    </message>
    <message>
        <location filename="../sinkwidget.cc" line="110"/>
        <source>pa_context_set_default_sink() failed</source>
        <translation>pa_context_set_default_sink() mislyktes</translation>
    </message>
    <message>
        <location filename="../sinkwidget.cc" line="126"/>
        <source>pa_context_set_sink_port_by_index() failed</source>
        <translation>pa_context_set_sink_port_by_index() mislyktes</translation>
    </message>
    <message>
        <location filename="../sinkwidget.cc" line="166"/>
        <source>pa_ext_device_restore_save_sink_formats() failed</source>
        <translation>pa_ext_device_restore_save_sink_formats() mislyktes</translation>
    </message>
</context>
<context>
    <name>SourceOutputWidget</name>
    <message>
        <location filename="../sourceoutputwidget.cc" line="35"/>
        <source>from</source>
        <translation>fra</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="39"/>
        <source>Terminate Recording</source>
        <translation>Avslutt opptak</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="60"/>
        <source>Unknown input</source>
        <translation>Ukjent inngang</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="72"/>
        <source>pa_context_set_source_output_volume() failed</source>
        <translation>pa_context_set_source_output_volume() mislyktes</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="87"/>
        <source>pa_context_set_source_output_mute() failed</source>
        <translation>pa_context_set_source_output_mute() mislyktes</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="98"/>
        <source>pa_context_kill_source_output() failed</source>
        <translation>pa_context_kill_source_output() mislyktes</translation>
    </message>
    <message>
        <location filename="../sourceoutputwidget.cc" line="125"/>
        <source>pa_context_move_source_output_by_index() failed</source>
        <translation>pa_context_move_source_output_by_index() mislyktes</translation>
    </message>
</context>
<context>
    <name>SourceWidget</name>
    <message>
        <location filename="../sourcewidget.cc" line="35"/>
        <source>pa_context_set_source_volume_by_index() failed</source>
        <translation>pa_context_set_source_volume_by_index() mislyktes</translation>
    </message>
    <message>
        <location filename="../sourcewidget.cc" line="50"/>
        <source>pa_context_set_source_mute_by_index() failed</source>
        <translation>pa_context_set_source_mute_by_index() mislyktes</translation>
    </message>
    <message>
        <location filename="../sourcewidget.cc" line="64"/>
        <source>pa_context_set_default_source() failed</source>
        <translation>pa_context_set_default_source() mislyktes</translation>
    </message>
    <message>
        <location filename="../sourcewidget.cc" line="80"/>
        <source>pa_context_set_source_port_by_index() failed</source>
        <translation>pa_context_set_source_port_by_index() mislyktes</translation>
    </message>
</context>
<context>
    <name>StreamWidget</name>
    <message>
        <location filename="../streamwidget.ui" line="14"/>
        <source>Form</source>
        <translation>Formular</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="32"/>
        <source>Device Title</source>
        <translation>Enhetsnavn</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="52"/>
        <source>direction</source>
        <translation>retning</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="59"/>
        <source>device</source>
        <translation>enhet</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="66"/>
        <source>Mute audio</source>
        <translation>Skru av lyd</translation>
    </message>
    <message>
        <location filename="../streamwidget.ui" line="79"/>
        <source>Lock channels together</source>
        <translation>Lås kanaler sammen</translation>
    </message>
    <message>
        <location filename="../streamwidget.cc" line="34"/>
        <source>Terminate</source>
        <translation>Avslutt</translation>
    </message>
</context>
</TS>

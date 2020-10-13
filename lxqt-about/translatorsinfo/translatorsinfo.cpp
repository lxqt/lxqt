/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */


#include "translatorsinfo.h"
#include <QDebug>
#include <QSettings>
#include <QStringList>
#include <QTextCodec>
#include <QTextDocument>
#include <QHash>

//using namespace LXQt;

void fillLangguages(QHash<QString, QString> *languages)
{
    languages->insert(QStringLiteral("ach")   ,QStringLiteral("Acoli"));
    languages->insert(QStringLiteral("af")    ,QStringLiteral("Afrikaans"));
    languages->insert(QStringLiteral("af_ZA") ,QStringLiteral("Afrikaans (South Africa)"));
    languages->insert(QStringLiteral("ak")    ,QStringLiteral("Akan"));
    languages->insert(QStringLiteral("sq")    ,QStringLiteral("Albanian"));
    languages->insert(QStringLiteral("sq_AL") ,QStringLiteral("Albanian (Albania)"));
    languages->insert(QStringLiteral("aln")   ,QStringLiteral("Albanian Gheg"));
    languages->insert(QStringLiteral("am")    ,QStringLiteral("Amharic"));
    languages->insert(QStringLiteral("am_ET") ,QStringLiteral("Amharic (Ethiopia)"));
    languages->insert(QStringLiteral("ar")    ,QStringLiteral("Arabic"));
    languages->insert(QStringLiteral("ar_SA") ,QStringLiteral("Arabic (Saudi Arabia)"));
    languages->insert(QStringLiteral("ar_AA") ,QStringLiteral("Arabic (Unitag)"));
    languages->insert(QStringLiteral("an")    ,QStringLiteral("Aragonese"));
    languages->insert(QStringLiteral("hy")    ,QStringLiteral("Armenian"));
    languages->insert(QStringLiteral("hy_AM") ,QStringLiteral("Armenian (Armenia)"));
    languages->insert(QStringLiteral("as")    ,QStringLiteral("Assamese"));
    languages->insert(QStringLiteral("as_IN") ,QStringLiteral("Assamese (India)"));
    languages->insert(QStringLiteral("ast")   ,QStringLiteral("Asturian"));
    languages->insert(QStringLiteral("az")    ,QStringLiteral("Azerbaijani"));
    languages->insert(QStringLiteral("az_AZ") ,QStringLiteral("Azerbaijani (Azerbaijan)"));
    languages->insert(QStringLiteral("bal")   ,QStringLiteral("Balochi"));
    languages->insert(QStringLiteral("eu")    ,QStringLiteral("Basque"));
    languages->insert(QStringLiteral("eu_ES") ,QStringLiteral("Basque (Spain)"));
    languages->insert(QStringLiteral("be")    ,QStringLiteral("Belarusian"));
    languages->insert(QStringLiteral("be_BY") ,QStringLiteral("Belarusian (Belarus)"));
    languages->insert(QStringLiteral("be@tarask")     ,QStringLiteral("Belarusian (Tarask)"));
    languages->insert(QStringLiteral("bn")    ,QStringLiteral("Bengali"));
    languages->insert(QStringLiteral("bn_BD") ,QStringLiteral("Bengali (Bangladesh)"));
    languages->insert(QStringLiteral("bn_IN") ,QStringLiteral("Bengali (India)"));
    languages->insert(QStringLiteral("brx")   ,QStringLiteral("Bodo"));
    languages->insert(QStringLiteral("bs")    ,QStringLiteral("Bosnian"));
    languages->insert(QStringLiteral("bs_BA") ,QStringLiteral("Bosnian (Bosnia and Herzegovina)"));
    languages->insert(QStringLiteral("br")    ,QStringLiteral("Breton"));
    languages->insert(QStringLiteral("bg")    ,QStringLiteral("Bulgarian"));
    languages->insert(QStringLiteral("bg_BG") ,QStringLiteral("Bulgarian (Bulgaria)"));
    languages->insert(QStringLiteral("my")    ,QStringLiteral("Burmese"));
    languages->insert(QStringLiteral("my_MM") ,QStringLiteral("Burmese (Myanmar)"));
    languages->insert(QStringLiteral("ca")    ,QStringLiteral("Catalan"));
    languages->insert(QStringLiteral("ca_ES") ,QStringLiteral("Catalan (Spain)"));
    languages->insert(QStringLiteral("ca@valencia")   ,QStringLiteral("Catalan (Valencian)"));
    languages->insert(QStringLiteral("hne")   ,QStringLiteral("Chhattisgarhi"));
    languages->insert(QStringLiteral("cgg")   ,QStringLiteral("Chiga"));
    languages->insert(QStringLiteral("zh")    ,QStringLiteral("Chinese"));
    languages->insert(QStringLiteral("zh_CN") ,QStringLiteral("Chinese (China)"));
    languages->insert(QStringLiteral("zh_CN.GB2312")  ,QStringLiteral("Chinese (China) (GB2312)"));
    languages->insert(QStringLiteral("zh_HK") ,QStringLiteral("Chinese (Hong Kong)"));
    languages->insert(QStringLiteral("zh_TW") ,QStringLiteral("Chinese (Taiwan)"));
    languages->insert(QStringLiteral("zh_TW.Big5")    ,QStringLiteral("Chinese (Taiwan) (Big5) "));
    languages->insert(QStringLiteral("kw")    ,QStringLiteral("Cornish"));
    languages->insert(QStringLiteral("co")    ,QStringLiteral("Corsican"));
    languages->insert(QStringLiteral("crh")   ,QStringLiteral("Crimean Turkish"));
    languages->insert(QStringLiteral("hr")    ,QStringLiteral("Croatian"));
    languages->insert(QStringLiteral("hr_HR") ,QStringLiteral("Croatian (Croatia)"));
    languages->insert(QStringLiteral("cs")    ,QStringLiteral("Czech"));
    languages->insert(QStringLiteral("cs_CZ") ,QStringLiteral("Czech (Czech Republic)"));
    languages->insert(QStringLiteral("da")    ,QStringLiteral("Danish"));
    languages->insert(QStringLiteral("nl")    ,QStringLiteral("Dutch"));
    languages->insert(QStringLiteral("nl_BE") ,QStringLiteral("Dutch (Belgium)"));
    languages->insert(QStringLiteral("nl_NL") ,QStringLiteral("Dutch (Netherlands)"));
    languages->insert(QStringLiteral("dz")    ,QStringLiteral("Dzongkha"));
    languages->insert(QStringLiteral("dz_BT") ,QStringLiteral("Dzongkha (Bhutan)"));
    languages->insert(QStringLiteral("en")    ,QStringLiteral("English"));
    languages->insert(QStringLiteral("en_AU") ,QStringLiteral("English (Australia)"));
    languages->insert(QStringLiteral("en_CA") ,QStringLiteral("English (Canada)"));
    languages->insert(QStringLiteral("en_IE") ,QStringLiteral("English (Ireland)"));
    languages->insert(QStringLiteral("en_ZA") ,QStringLiteral("English (South Africa)"));
    languages->insert(QStringLiteral("en_GB") ,QStringLiteral("English (United Kingdom)"));
    languages->insert(QStringLiteral("en_US") ,QStringLiteral("English (United States)"));
    languages->insert(QStringLiteral("eo")    ,QStringLiteral("Esperanto"));
    languages->insert(QStringLiteral("et")    ,QStringLiteral("Estonian"));
    languages->insert(QStringLiteral("et_EE") ,QStringLiteral("Estonian (Estonia)"));
    languages->insert(QStringLiteral("fo")    ,QStringLiteral("Faroese"));
    languages->insert(QStringLiteral("fo_FO") ,QStringLiteral("Faroese (Faroe Islands)"));
    languages->insert(QStringLiteral("fil")   ,QStringLiteral("Filipino"));
    languages->insert(QStringLiteral("fi")    ,QStringLiteral("Finnish"));
    languages->insert(QStringLiteral("fi_FI") ,QStringLiteral("Finnish (Finland)"));
    languages->insert(QStringLiteral("frp")   ,QStringLiteral("Franco-Provençal (Arpitan)"));
    languages->insert(QStringLiteral("fr")    ,QStringLiteral("French"));
    languages->insert(QStringLiteral("fr_CA") ,QStringLiteral("French (Canada)"));
    languages->insert(QStringLiteral("fr_FR") ,QStringLiteral("French (France)"));
    languages->insert(QStringLiteral("fr_CH") ,QStringLiteral("French (Switzerland)"));
    languages->insert(QStringLiteral("fur")   ,QStringLiteral("Friulian"));
    languages->insert(QStringLiteral("ff")    ,QStringLiteral("Fulah"));
    languages->insert(QStringLiteral("gd")    ,QStringLiteral("Gaelic, Scottish"));
    languages->insert(QStringLiteral("gl")    ,QStringLiteral("Galician"));
    languages->insert(QStringLiteral("gl_ES") ,QStringLiteral("Galician (Spain)"));
    languages->insert(QStringLiteral("lg")    ,QStringLiteral("Ganda"));
    languages->insert(QStringLiteral("ka")    ,QStringLiteral("Georgian"));
    languages->insert(QStringLiteral("ka_GE") ,QStringLiteral("Georgian (Georgia)"));
    languages->insert(QStringLiteral("de")    ,QStringLiteral("German"));
    languages->insert(QStringLiteral("de_DE") ,QStringLiteral("German (Germany)"));
    languages->insert(QStringLiteral("de_CH") ,QStringLiteral("German (Switzerland)"));
    languages->insert(QStringLiteral("el")    ,QStringLiteral("Greek"));
    languages->insert(QStringLiteral("el_GR") ,QStringLiteral("Greek (Greece)"));
    languages->insert(QStringLiteral("gu")    ,QStringLiteral("Gujarati"));
    languages->insert(QStringLiteral("gu_IN") ,QStringLiteral("Gujarati (India)"));
    languages->insert(QStringLiteral("gun")   ,QStringLiteral("Gun"));
    languages->insert(QStringLiteral("ht")    ,QStringLiteral("Haitian (Haitian Creole)"));
    languages->insert(QStringLiteral("ht_HT") ,QStringLiteral("Haitian (Haitian Creole) (Haiti)"));
    languages->insert(QStringLiteral("ha")    ,QStringLiteral("Hausa"));
    languages->insert(QStringLiteral("he")    ,QStringLiteral("Hebrew"));
    languages->insert(QStringLiteral("he_IL") ,QStringLiteral("Hebrew (Israel)"));
    languages->insert(QStringLiteral("hi")    ,QStringLiteral("Hindi"));
    languages->insert(QStringLiteral("hi_IN") ,QStringLiteral("Hindi (India)"));
    languages->insert(QStringLiteral("hu")    ,QStringLiteral("Hungarian"));
    languages->insert(QStringLiteral("hu_HU") ,QStringLiteral("Hungarian (Hungary)"));
    languages->insert(QStringLiteral("is")    ,QStringLiteral("Icelandic"));
    languages->insert(QStringLiteral("is_IS") ,QStringLiteral("Icelandic (Iceland)"));
    languages->insert(QStringLiteral("ig")    ,QStringLiteral("Igbo"));
    languages->insert(QStringLiteral("ilo")   ,QStringLiteral("Iloko"));
    languages->insert(QStringLiteral("id")    ,QStringLiteral("Indonesian"));
    languages->insert(QStringLiteral("id_ID") ,QStringLiteral("Indonesian (Indonesia)"));
    languages->insert(QStringLiteral("ia")    ,QStringLiteral("Interlingua"));
    languages->insert(QStringLiteral("ga")    ,QStringLiteral("Irish"));
    languages->insert(QStringLiteral("ga_IE") ,QStringLiteral("Irish (Ireland)"));
    languages->insert(QStringLiteral("it")    ,QStringLiteral("Italian"));
    languages->insert(QStringLiteral("it_IT") ,QStringLiteral("Italian (Italy)"));
    languages->insert(QStringLiteral("ja")    ,QStringLiteral("Japanese"));
    languages->insert(QStringLiteral("ja_JP") ,QStringLiteral("Japanese (Japan)"));
    languages->insert(QStringLiteral("jv")    ,QStringLiteral("Javanese"));
    languages->insert(QStringLiteral("kn")    ,QStringLiteral("Kannada"));
    languages->insert(QStringLiteral("kn_IN") ,QStringLiteral("Kannada (India)"));
    languages->insert(QStringLiteral("ks")    ,QStringLiteral("Kashmiri"));
    languages->insert(QStringLiteral("ks_IN") ,QStringLiteral("Kashmiri (India)"));
    languages->insert(QStringLiteral("csb")   ,QStringLiteral("Kashubian"));
    languages->insert(QStringLiteral("kk")    ,QStringLiteral("Kazakh"));
    languages->insert(QStringLiteral("kk_KZ") ,QStringLiteral("Kazakh (Kazakhstan)"));
    languages->insert(QStringLiteral("km")    ,QStringLiteral("Khmer"));
    languages->insert(QStringLiteral("km_KH") ,QStringLiteral("Khmer (Cambodia)"));
    languages->insert(QStringLiteral("rw")    ,QStringLiteral("Kinyarwanda"));
    languages->insert(QStringLiteral("ky")    ,QStringLiteral("Kirgyz"));
    languages->insert(QStringLiteral("tlh")   ,QStringLiteral("Klingon"));
    languages->insert(QStringLiteral("ko")    ,QStringLiteral("Korean"));
    languages->insert(QStringLiteral("ko_KR") ,QStringLiteral("Korean (Korea)"));
    languages->insert(QStringLiteral("ku")    ,QStringLiteral("Kurdish"));
    languages->insert(QStringLiteral("ku_IQ") ,QStringLiteral("Kurdish (Iraq)"));
    languages->insert(QStringLiteral("lo")    ,QStringLiteral("Lao"));
    languages->insert(QStringLiteral("lo_LA") ,QStringLiteral("Lao (Laos)"));
    languages->insert(QStringLiteral("la")    ,QStringLiteral("Latin"));
    languages->insert(QStringLiteral("lv")    ,QStringLiteral("Latvian"));
    languages->insert(QStringLiteral("lv_LV") ,QStringLiteral("Latvian (Latvia)"));
    languages->insert(QStringLiteral("li")    ,QStringLiteral("Limburgian"));
    languages->insert(QStringLiteral("ln")    ,QStringLiteral("Lingala"));
    languages->insert(QStringLiteral("lt")    ,QStringLiteral("Lithuanian"));
    languages->insert(QStringLiteral("lt_LT") ,QStringLiteral("Lithuanian (Lithuania)"));
    languages->insert(QStringLiteral("nds")   ,QStringLiteral("Low German"));
    languages->insert(QStringLiteral("lb")    ,QStringLiteral("Luxembourgish"));
    languages->insert(QStringLiteral("mk")    ,QStringLiteral("Macedonian"));
    languages->insert(QStringLiteral("mk_MK") ,QStringLiteral("Macedonian (Macedonia)"));
    languages->insert(QStringLiteral("mai")   ,QStringLiteral("Maithili"));
    languages->insert(QStringLiteral("mg")    ,QStringLiteral("Malagasy"));
    languages->insert(QStringLiteral("ms")    ,QStringLiteral("Malay"));
    languages->insert(QStringLiteral("ml")    ,QStringLiteral("Malayalam"));
    languages->insert(QStringLiteral("ml_IN") ,QStringLiteral("Malayalam (India)"));
    languages->insert(QStringLiteral("ms_MY") ,QStringLiteral("Malay (Malaysia)"));
    languages->insert(QStringLiteral("mt")    ,QStringLiteral("Maltese"));
    languages->insert(QStringLiteral("mt_MT") ,QStringLiteral("Maltese (Malta)"));
    languages->insert(QStringLiteral("mi")    ,QStringLiteral("Maori"));
    languages->insert(QStringLiteral("arn")   ,QStringLiteral("Mapudungun"));
    languages->insert(QStringLiteral("mr")    ,QStringLiteral("Marathi"));
    languages->insert(QStringLiteral("mr_IN") ,QStringLiteral("Marathi (India)"));
    languages->insert(QStringLiteral("mn")    ,QStringLiteral("Mongolian"));
    languages->insert(QStringLiteral("mn_MN") ,QStringLiteral("Mongolian (Mongolia)"));
    languages->insert(QStringLiteral("nah")   ,QStringLiteral("Nahuatl"));
    languages->insert(QStringLiteral("nr")    ,QStringLiteral("Ndebele, South"));
    languages->insert(QStringLiteral("nap")   ,QStringLiteral("Neapolitan"));
    languages->insert(QStringLiteral("ne")    ,QStringLiteral("Nepali"));
    languages->insert(QStringLiteral("ne_NP") ,QStringLiteral("Nepali (Nepal)"));
    languages->insert(QStringLiteral("se")    ,QStringLiteral("Northern Sami"));
    languages->insert(QStringLiteral("nso")   ,QStringLiteral("Northern Sotho"));
    languages->insert(QStringLiteral("no")    ,QStringLiteral("Norwegian"));
    languages->insert(QStringLiteral("nb")    ,QStringLiteral("Norwegian Bokmål"));
    languages->insert(QStringLiteral("nb_NO") ,QStringLiteral("Norwegian Bokmål (Norway)"));
    languages->insert(QStringLiteral("no_NO") ,QStringLiteral("Norwegian (Norway)"));
    languages->insert(QStringLiteral("nn")    ,QStringLiteral("Norwegian Nynorsk"));
    languages->insert(QStringLiteral("nn_NO") ,QStringLiteral("Norwegian Nynorsk (Norway)"));
    languages->insert(QStringLiteral("ny")    ,QStringLiteral("Nyanja"));
    languages->insert(QStringLiteral("oc")    ,QStringLiteral("Occitan (post 1500)"));
    languages->insert(QStringLiteral("or")    ,QStringLiteral("Oriya"));
    languages->insert(QStringLiteral("or_IN") ,QStringLiteral("Oriya (India)"));
    languages->insert(QStringLiteral("pa")    ,QStringLiteral("Panjabi (Punjabi)"));
    languages->insert(QStringLiteral("pa_IN") ,QStringLiteral("Panjabi (Punjabi) (India)"));
    languages->insert(QStringLiteral("pap")   ,QStringLiteral("Papiamento"));
    languages->insert(QStringLiteral("fa")    ,QStringLiteral("Persian"));
    languages->insert(QStringLiteral("fa_IR") ,QStringLiteral("Persian (Iran)"));
    languages->insert(QStringLiteral("pms")   ,QStringLiteral("Piemontese"));
    languages->insert(QStringLiteral("pl")    ,QStringLiteral("Polish"));
    languages->insert(QStringLiteral("pl_PL") ,QStringLiteral("Polish (Poland)"));
    languages->insert(QStringLiteral("pt")    ,QStringLiteral("Portuguese"));
    languages->insert(QStringLiteral("pt_BR") ,QStringLiteral("Portuguese (Brazil)"));
    languages->insert(QStringLiteral("pt_PT") ,QStringLiteral("Portuguese (Portugal)"));
    languages->insert(QStringLiteral("ps")    ,QStringLiteral("Pushto"));
    languages->insert(QStringLiteral("ro")    ,QStringLiteral("Romanian"));
    languages->insert(QStringLiteral("ro_RO") ,QStringLiteral("Romanian (Romania)"));
    languages->insert(QStringLiteral("rm")    ,QStringLiteral("Romansh"));
    languages->insert(QStringLiteral("ru")    ,QStringLiteral("Russian"));
    languages->insert(QStringLiteral("ru_RU") ,QStringLiteral("Russian (Russia)"));
    languages->insert(QStringLiteral("sm")    ,QStringLiteral("Samoan"));
    languages->insert(QStringLiteral("sc")    ,QStringLiteral("Sardinian"));
    languages->insert(QStringLiteral("sco")   ,QStringLiteral("Scots"));
    languages->insert(QStringLiteral("sr")    ,QStringLiteral("Serbian"));
    languages->insert(QStringLiteral("sr@latin")      ,QStringLiteral("Serbian (Latin)"));
    languages->insert(QStringLiteral("sr_RS@latin")   ,QStringLiteral("Serbian (Latin) (Serbia)"));
    languages->insert(QStringLiteral("sr_RS") ,QStringLiteral("Serbian (Serbia)"));
    languages->insert(QStringLiteral("sn")    ,QStringLiteral("Shona"));
    languages->insert(QStringLiteral("sd")    ,QStringLiteral("Sindhi"));
    languages->insert(QStringLiteral("si")    ,QStringLiteral("Sinhala"));
    languages->insert(QStringLiteral("si_LK") ,QStringLiteral("Sinhala (Sri Lanka)"));
    languages->insert(QStringLiteral("sk")    ,QStringLiteral("Slovak"));
    languages->insert(QStringLiteral("sk_SK") ,QStringLiteral("Slovak (Slovakia)"));
    languages->insert(QStringLiteral("sl")    ,QStringLiteral("Slovenian"));
    languages->insert(QStringLiteral("sl_SI") ,QStringLiteral("Slovenian (Slovenia)"));
    languages->insert(QStringLiteral("so")    ,QStringLiteral("Somali"));
    languages->insert(QStringLiteral("son")   ,QStringLiteral("Songhay"));
    languages->insert(QStringLiteral("st")    ,QStringLiteral("Sotho, Southern"));
    languages->insert(QStringLiteral("st_ZA") ,QStringLiteral("Sotho, Southern (South Africa)"));
    languages->insert(QStringLiteral("es")    ,QStringLiteral("Spanish"));
    languages->insert(QStringLiteral("es_AR") ,QStringLiteral("Spanish (Argentina)"));
    languages->insert(QStringLiteral("es_BO") ,QStringLiteral("Spanish (Bolivia)"));
    languages->insert(QStringLiteral("es_CL") ,QStringLiteral("Spanish (Chile)"));
    languages->insert(QStringLiteral("es_CO") ,QStringLiteral("Spanish (Colombia)"));
    languages->insert(QStringLiteral("es_CR") ,QStringLiteral("Spanish (Costa Rica)"));
    languages->insert(QStringLiteral("es_DO") ,QStringLiteral("Spanish (Dominican Republic)"));
    languages->insert(QStringLiteral("es_EC") ,QStringLiteral("Spanish (Ecuador)"));
    languages->insert(QStringLiteral("es_SV") ,QStringLiteral("Spanish (El Salvador)"));
    languages->insert(QStringLiteral("es_MX") ,QStringLiteral("Spanish (Mexico)"));
    languages->insert(QStringLiteral("es_NI") ,QStringLiteral("Spanish (Nicaragua)"));
    languages->insert(QStringLiteral("es_PA") ,QStringLiteral("Spanish (Panama)"));
    languages->insert(QStringLiteral("es_PY") ,QStringLiteral("Spanish (Paraguay)"));
    languages->insert(QStringLiteral("es_PE") ,QStringLiteral("Spanish (Peru)"));
    languages->insert(QStringLiteral("es_PR") ,QStringLiteral("Spanish (Puerto Rico)"));
    languages->insert(QStringLiteral("es_ES") ,QStringLiteral("Spanish (Spain)"));
    languages->insert(QStringLiteral("es_UY") ,QStringLiteral("Spanish (Uruguay)"));
    languages->insert(QStringLiteral("es_VE") ,QStringLiteral("Spanish (Venezuela)"));
    languages->insert(QStringLiteral("su")    ,QStringLiteral("Sundanese"));
    languages->insert(QStringLiteral("sw")    ,QStringLiteral("Swahili"));
    languages->insert(QStringLiteral("sw_KE") ,QStringLiteral("Swahili (Kenya)"));
    languages->insert(QStringLiteral("sv")    ,QStringLiteral("Swedish"));
    languages->insert(QStringLiteral("sv_FI") ,QStringLiteral("Swedish (Finland)"));
    languages->insert(QStringLiteral("sv_SE") ,QStringLiteral("Swedish (Sweden)"));
    languages->insert(QStringLiteral("tl")    ,QStringLiteral("Tagalog"));
    languages->insert(QStringLiteral("tl_PH") ,QStringLiteral("Tagalog (Philippines)"));
    languages->insert(QStringLiteral("tg")    ,QStringLiteral("Tajik"));
    languages->insert(QStringLiteral("tg_TJ") ,QStringLiteral("Tajik (Tajikistan)"));
    languages->insert(QStringLiteral("ta")    ,QStringLiteral("Tamil"));
    languages->insert(QStringLiteral("ta_IN") ,QStringLiteral("Tamil (India)"));
    languages->insert(QStringLiteral("ta_LK") ,QStringLiteral("Tamil (Sri-Lanka)"));
    languages->insert(QStringLiteral("tt")    ,QStringLiteral("Tatar"));
    languages->insert(QStringLiteral("te")    ,QStringLiteral("Telugu"));
    languages->insert(QStringLiteral("te_IN") ,QStringLiteral("Telugu (India)"));
    languages->insert(QStringLiteral("th")    ,QStringLiteral("Thai"));
    languages->insert(QStringLiteral("th_TH") ,QStringLiteral("Thai (Thailand)"));
    languages->insert(QStringLiteral("bo")    ,QStringLiteral("Tibetan"));
    languages->insert(QStringLiteral("bo_CN") ,QStringLiteral("Tibetan (China)"));
    languages->insert(QStringLiteral("ti")    ,QStringLiteral("Tigrinya"));
    languages->insert(QStringLiteral("to")    ,QStringLiteral("Tongan"));
    languages->insert(QStringLiteral("tr")    ,QStringLiteral("Turkish"));
    languages->insert(QStringLiteral("tr_TR") ,QStringLiteral("Turkish (Turkey)"));
    languages->insert(QStringLiteral("tk")    ,QStringLiteral("Turkmen"));
    languages->insert(QStringLiteral("ug")    ,QStringLiteral("Uighur"));
    languages->insert(QStringLiteral("uk")    ,QStringLiteral("Ukrainian"));
    languages->insert(QStringLiteral("uk_UA") ,QStringLiteral("Ukrainian (Ukraine)"));
    languages->insert(QStringLiteral("hsb")   ,QStringLiteral("Upper Sorbian"));
    languages->insert(QStringLiteral("ur")    ,QStringLiteral("Urdu"));
    languages->insert(QStringLiteral("ur_PK") ,QStringLiteral("Urdu (Pakistan)"));
    languages->insert(QStringLiteral("uz")    ,QStringLiteral("Uzbek"));
    languages->insert(QStringLiteral("ve")    ,QStringLiteral("Venda"));
    languages->insert(QStringLiteral("vi")    ,QStringLiteral("Vietnamese"));
    languages->insert(QStringLiteral("vi_VN") ,QStringLiteral("Vietnamese (Vietnam)"));
    languages->insert(QStringLiteral("vls")   ,QStringLiteral("Vlaams"));
    languages->insert(QStringLiteral("wa")    ,QStringLiteral("Walloon"));
    languages->insert(QStringLiteral("cy")    ,QStringLiteral("Welsh"));
    languages->insert(QStringLiteral("cy_GB") ,QStringLiteral("Welsh (United Kingdom)"));
    languages->insert(QStringLiteral("fy")    ,QStringLiteral("Western Frisian"));
    languages->insert(QStringLiteral("fy_NL") ,QStringLiteral("Western Frisian (Netherlands)"));
    languages->insert(QStringLiteral("wo")    ,QStringLiteral("Wolof"));
    languages->insert(QStringLiteral("wo_SN") ,QStringLiteral("Wolof (Senegal)"));
    languages->insert(QStringLiteral("xh")    ,QStringLiteral("Xhosa"));
    languages->insert(QStringLiteral("yi")    ,QStringLiteral("Yiddish"));
    languages->insert(QStringLiteral("yo")    ,QStringLiteral("Yoruba"));
    languages->insert(QStringLiteral("zu")    ,QStringLiteral("Zulu"));
    languages->insert(QStringLiteral("zu_ZA") ,QStringLiteral("Zulu (South Africa)"));
}

QString languageToString(QString langId)
{
    static QHash<QString, QString> mLanguagesList;
    if (mLanguagesList.isEmpty())
    {
        fillLangguages(&mLanguagesList);
    }

    if (mLanguagesList.contains(langId)) {
        return mLanguagesList.value(langId);
    } else {
        return langId;
    }
}



QString getValue(const QSettings &src, const QString &key)
{
    QString ret = src.value(key).toString().trimmed();
    if (ret == QLatin1String("-"))
        return QLatin1String("");

    return ret;
}



TranslatorsInfo::TranslatorsInfo()
{
    //fillLangguages(&mLanguagesList);

    QSettings src(QStringLiteral(":/translatorsInfo"), QSettings::IniFormat);
    src.setIniCodec("UTF-8");

    const auto groups = src.childGroups();
    for(const QString& group : qAsConst(groups))
    {
        QString lang = group.section(QStringLiteral("_"), 1).remove(QStringLiteral(".info"));
        src.beginGroup(group);
        int cnt = src.allKeys().count();
        for (int i=0; i<cnt; i++)
        {
            QString nameEnglish = getValue(src, QStringLiteral("translator_%1_nameEnglish").arg(i));
            QString nameNative = getValue(src, QStringLiteral("translator_%1_nameNative").arg(i));
            QString contact = getValue(src, QStringLiteral("translator_%1_contact").arg(i));

            if (!nameEnglish.isEmpty())
            {
                process(lang, nameEnglish, nameNative, contact);
            }

        }
        src.endGroup();
    }
}

QString TranslatorsInfo::asHtml() const
{
    QString ret;
    ret = QLatin1String("<dl>");
    for(const auto& entry : mLangTranslators)
    {
        const auto& lang = entry.first;
        const auto& translators = entry.second;

        ret += QLatin1String("<dt><strong>") + languageToString(lang) + QLatin1String("</strong></dt>");
        for(const auto & translator : translators) {
            ret += QLatin1String("<dd>") + translator.asHtml() + QLatin1String("</dd>");
        }
    }
    ret += QLatin1String("</dl>");

    return ret;
}


void TranslatorsInfo::process(const QString &lang, const QString &englishName, const QString &nativeName, const QString &contact)
{
    mLangTranslators[lang].emplace(englishName, nativeName, contact);
}

QString TranslatorPerson::asHtml() const
{
    QString ret;
    if (mNativeName.isEmpty())
        ret = QStringLiteral("%1").arg(mEnglishName);
    else
        ret = QStringLiteral("%1 (%2)").arg(mEnglishName, mNativeName);

    if (!mContact.isEmpty())
    {
        if (mContact.contains(QRegExp(QStringLiteral("^(https?|mailto):"))))
            ret = QStringLiteral(" <a href='%1'>%2</a>").arg(mContact, ret.toHtmlEscaped());
        else if (mContact.contains(QLatin1String("@")) || mContact.contains(QLatin1String("<")))
            ret = QStringLiteral(" <a href='mailto:%1'>%2</a>").arg(mContact, ret.toHtmlEscaped());
        else
            ret = QStringLiteral(" <a href='http://%1'>%2</a>").arg(mContact, ret.toHtmlEscaped());
    }

    return ret;
}


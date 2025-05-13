#include "./lv_i18n.h"


////////////////////////////////////////////////////////////////////////////////
// Define plural operands
// http://unicode.org/reports/tr35/tr35-numbers.html#Operands

// Integer version, simplified

#define UNUSED(x) (void)(x)

static inline uint32_t op_n(int32_t val) { return (uint32_t)(val < 0 ? -val : val); }
static inline uint32_t op_i(uint32_t val) { return val; }
// always zero, when decimal part not exists.
static inline uint32_t op_v(uint32_t val) { UNUSED(val); return 0;}
static inline uint32_t op_w(uint32_t val) { UNUSED(val); return 0; }
static inline uint32_t op_f(uint32_t val) { UNUSED(val); return 0; }
static inline uint32_t op_t(uint32_t val) { UNUSED(val); return 0; }

static lv_i18n_phrase_t en_gb_singulars[] = {
    {"Chế độ mức nhiệt cố định", "User temperature set mode"},
    {"Đặt mức nhiệt", "Set the temperature"},
    {"50 độ C", "50 cencius degree"},
    {"Nhiệt độ bàn nhiệt", "Hotplate temperature"},
    {"Thoát", "Exit"},
    {"đang làm nóng ...", "Heating..."},
    {"Chức năng", "Mode"},
    {"Cài đặt", "Settings"},
    {"Lựa chọn chế độ làm việc", "Select work mode"},
    {"Cố định mức nhiệt\\nKịch bản sẵn có\\nKịch bản tùy biến", "'Static temp\nPre-set works scenarios\nCustomized works scenarios'"},
    {"Mức nhiệt được cố định theo mức cài đặt của người dùng.", "Temperature set by user"},
    {" độ C", " Celsius degree"},
    {"Mức nhiệt được cài đặt theo kịch bản có sẵn.", "Temperature set by pre-set scenarios"},
    {"Mức nhiệt được lên kịch bản theo cách sử dụng của người dùng.", "Temperature set by customized scenarios"},
    {NULL, NULL} // End mark
};



static uint8_t en_gb_plural_fn(int32_t num)
{
    uint32_t n = op_n(num); UNUSED(n);
    uint32_t i = op_i(n); UNUSED(i);
    uint32_t v = op_v(n); UNUSED(v);

    if ((i == 1 && v == 0)) return LV_I18N_PLURAL_TYPE_ONE;
    return LV_I18N_PLURAL_TYPE_OTHER;
}

static const lv_i18n_lang_t en_gb_lang = {
    .locale_name = "en-GB",
    .singulars = en_gb_singulars,

    .locale_plural_fn = en_gb_plural_fn
};

static lv_i18n_phrase_t vi_vn_singulars[] = {
    {"Chế độ mức nhiệt cố định", "Chế độ mức nhiệt cố định"},
    {"Đặt mức nhiệt", "Đặt mức nhiệt"},
    {"50 độ C", "50 độ C"},
    {"Nhiệt độ bàn nhiệt", "Nhiệt độ bàn nhiệt"},
    {"Thoát", "Thoát"},
    {"đang làm nóng ...", "đang làm nóng ..."},
    {"Chức năng", "Chức năng"},
    {"Cài đặt", "Cài đặt"},
    {"Lựa chọn chế độ làm việc", "Lựa chọn chế độ làm việc"},
    {"Cố định mức nhiệt\\nKịch bản sẵn có\\nKịch bản tùy biến", "Cố định mức nhiệt\nKịch bản sẵn có\nKịch bản tùy biến"},
    {"Mức nhiệt được cố định theo mức cài đặt của người dùng.", "Mức nhiệt được cố định theo mức cài đặt của người dùng."},
    {" độ C", " độ C"},
    {"Mức nhiệt được cài đặt theo kịch bản có sẵn.", "Mức nhiệt được cài đặt theo kịch bản có sẵn."},
    {"Mức nhiệt được lên kịch bản theo cách sử dụng của người dùng.", "Mức nhiệt được lên kịch bản theo cách sử dụng của người dùng."},
    {NULL, NULL} // End mark
};



static uint8_t vi_vn_plural_fn(int32_t num)
{



    return LV_I18N_PLURAL_TYPE_OTHER;
}

static const lv_i18n_lang_t vi_vn_lang = {
    .locale_name = "vi-VN",
    .singulars = vi_vn_singulars,

    .locale_plural_fn = vi_vn_plural_fn
};

const lv_i18n_language_pack_t lv_i18n_language_pack[] = {
    &en_gb_lang,
    &vi_vn_lang,
    NULL // End mark
};

////////////////////////////////////////////////////////////////////////////////


// Internal state
static const lv_i18n_language_pack_t * current_lang_pack;
static const lv_i18n_lang_t * current_lang;


/**
 * Reset internal state. For testing.
 */
void __lv_i18n_reset(void)
{
    current_lang_pack = NULL;
    current_lang = NULL;
}

/**
 * Set the languages for internationalization
 * @param langs pointer to the array of languages. (Last element has to be `NULL`)
 */
int lv_i18n_init(const lv_i18n_language_pack_t * langs)
{
    if(langs == NULL) return -1;
    if(langs[0] == NULL) return -1;

    current_lang_pack = langs;
    current_lang = langs[0];     /*Automatically select the first language*/
    return 0;
}

/**
 * Change the localization (language)
 * @param l_name name of the translation locale to use. E.g. "en-GB"
 */
int lv_i18n_set_locale(const char * l_name)
{
    if(current_lang_pack == NULL) return -1;

    uint16_t i;

    for(i = 0; current_lang_pack[i] != NULL; i++) {
        // Found -> finish
        if(strcmp(current_lang_pack[i]->locale_name, l_name) == 0) {
            current_lang = current_lang_pack[i];
            return 0;
        }
    }

    return -1;
}


static const char * __lv_i18n_get_text_core(lv_i18n_phrase_t * trans, const char * msg_id)
{
    uint16_t i;
    for(i = 0; trans[i].msg_id != NULL; i++) {
        if(strcmp(trans[i].msg_id, msg_id) == 0) {
            /*The msg_id has found. Check the translation*/
            if(trans[i].translation) return trans[i].translation;
        }
    }

    return NULL;
}


/**
 * Get the translation from a message ID
 * @param msg_id message ID
 * @return the translation of `msg_id` on the set local
 */
const char * lv_i18n_get_text(const char * msg_id)
{
    if(current_lang == NULL) return msg_id;

    const lv_i18n_lang_t * lang = current_lang;
    const void * txt;

    // Search in current locale
    if(lang->singulars != NULL) {
        txt = __lv_i18n_get_text_core(lang->singulars, msg_id);
        if (txt != NULL) return txt;
    }

    // Try to fallback
    if(lang == current_lang_pack[0]) return msg_id;
    lang = current_lang_pack[0];

    // Repeat search for default locale
    if(lang->singulars != NULL) {
        txt = __lv_i18n_get_text_core(lang->singulars, msg_id);
        if (txt != NULL) return txt;
    }

    return msg_id;
}

/**
 * Get the translation from a message ID and apply the language's plural rule to get correct form
 * @param msg_id message ID
 * @param num an integer to select the correct plural form
 * @return the translation of `msg_id` on the set local
 */
const char * lv_i18n_get_text_plural(const char * msg_id, int32_t num)
{
    if(current_lang == NULL) return msg_id;

    const lv_i18n_lang_t * lang = current_lang;
    const void * txt;
    lv_i18n_plural_type_t ptype;

    // Search in current locale
    if(lang->locale_plural_fn != NULL) {
        ptype = lang->locale_plural_fn(num);

        if(lang->plurals[ptype] != NULL) {
            txt = __lv_i18n_get_text_core(lang->plurals[ptype], msg_id);
            if (txt != NULL) return txt;
        }
    }

    // Try to fallback
    if(lang == current_lang_pack[0]) return msg_id;
    lang = current_lang_pack[0];

    // Repeat search for default locale
    if(lang->locale_plural_fn != NULL) {
        ptype = lang->locale_plural_fn(num);

        if(lang->plurals[ptype] != NULL) {
            txt = __lv_i18n_get_text_core(lang->plurals[ptype], msg_id);
            if (txt != NULL) return txt;
        }
    }

    return msg_id;
}

/**
 * Get the name of the currently used locale.
 * @return name of the currently used locale. E.g. "en-GB"
 */
const char * lv_i18n_get_current_locale(void)
{
    if(!current_lang) return NULL;
    return current_lang->locale_name;
}

typedef struct
{
    unsigned char tag;
    unsigned char length;
    struct
    {
        unsigned char DCO_16MHZ;
        unsigned char BC1_16MHZ;
        unsigned char DCO_12MHZ;
        unsigned char BC1_12MHZ;
        unsigned char DCO_8MHZ;
        unsigned char BC1_8MHZ;
        unsigned char DCO_1MHZ;
        unsigned char BC1_1MHZ;
    } value;
} const dco_30_tag_t;

typedef struct
{
    unsigned char tag;
    unsigned char length;
    struct
    {
        unsigned int ADC_GAIN_FACTOR;
        unsigned int ADC_OFFSET;
        unsigned int ADC_15VREF_FACTOR;
        unsigned int ADC_15T30;
        unsigned int ADC_15T85;
        unsigned int ADC_25VREF_FACTOR;
        unsigned int ADC_25T30;
        unsigned int ADC_25T85;
    } value;
} const adc12_1_tag_t;

typedef struct
{
    unsigned char tag;
    unsigned char length;
} const empty_tag_t;

struct
{
    unsigned int checksum;
    empty_tag_t  empty;
    unsigned int dummy[11];
    adc12_1_tag_t adc12_1;
    dco_30_tag_t dco_30;
} const volatile TLV_bits asm("0x10c0");
#endif  // __ASSEMBLER__
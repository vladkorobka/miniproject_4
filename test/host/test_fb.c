#include <string.h>

#include "unity.h"
#include "fb.h"
#include "font8x8_basic.h"

void setUp(void) {}
void tearDown(void) {}

static fb_t fb;

static void assert_glyph_at(int x, int y, char c)
{
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            bool expected = (font8x8_basic[(unsigned char)c][row] >> col) & 1;
            TEST_ASSERT_EQUAL(expected, fb_get_pixel(&fb, x + col, y + 2 * row));
            TEST_ASSERT_EQUAL(expected, fb_get_pixel(&fb, x + col, y + 2 * row + 1));
        }
    }
}

static void test_clear_zeroes_buffer(void)
{
    memset(fb.buf, 0xAA, sizeof fb.buf);
    fb_clear(&fb);
    for (size_t i = 0; i < sizeof fb.buf; i++) {
        TEST_ASSERT_EQUAL_HEX8(0, fb.buf[i]);
    }
}

static void test_pixel_uses_ssd1306_page_layout(void)
{
    fb_clear(&fb);
    fb_set_pixel(&fb, 0, 0, true);
    TEST_ASSERT_EQUAL_HEX8(0x01, fb.buf[0]);
    fb_set_pixel(&fb, 5, 13, true);                 /* сторінка 1, біт 5 */
    TEST_ASSERT_EQUAL_HEX8(0x20, fb.buf[FB_WIDTH + 5]);
    TEST_ASSERT_TRUE(fb_get_pixel(&fb, 5, 13));
    fb_set_pixel(&fb, 5, 13, false);
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, 5, 13));
}

static void test_out_of_bounds_is_ignored(void)
{
    fb_clear(&fb);
    fb_set_pixel(&fb, -1, 0, true);
    fb_set_pixel(&fb, 0, -1, true);
    fb_set_pixel(&fb, FB_WIDTH, 0, true);
    fb_set_pixel(&fb, 0, FB_HEIGHT, true);
    for (size_t i = 0; i < sizeof fb.buf; i++) {
        TEST_ASSERT_EQUAL_HEX8(0, fb.buf[i]);
    }
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, FB_WIDTH, FB_HEIGHT));
}

static void test_draw_char_doubles_glyph_rows(void)
{
    fb_clear(&fb);
    fb_draw_char(&fb, 8, 16, 'A');
    assert_glyph_at(8, 16, 'A');
}

static void test_draw_char_replaces_non_ascii(void)
{
    fb_clear(&fb);
    fb_draw_char(&fb, 0, 0, '\n');
    assert_glyph_at(0, 0, '?');
}

static void test_draw_text_advances_and_clips(void)
{
    fb_clear(&fb);
    fb_draw_text(&fb, 0, 0, "ABCDEFGHIJKLMNOPQRSTU");   /* 21 символ, вміщується 16 */
    assert_glyph_at(0, 0, 'A');
    assert_glyph_at(15 * FB_CHAR_W, 0, 'P');
}

static void test_invert_rect(void)
{
    fb_clear(&fb);
    fb_invert_rect(&fb, 0, 16, FB_WIDTH, 16);
    TEST_ASSERT_TRUE(fb_get_pixel(&fb, 0, 16));
    TEST_ASSERT_TRUE(fb_get_pixel(&fb, 127, 31));
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, 127, 15));
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, 0, 32));
    fb_invert_rect(&fb, 0, 16, FB_WIDTH, 16);
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, 0, 16));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clear_zeroes_buffer);
    RUN_TEST(test_pixel_uses_ssd1306_page_layout);
    RUN_TEST(test_out_of_bounds_is_ignored);
    RUN_TEST(test_draw_char_doubles_glyph_rows);
    RUN_TEST(test_draw_char_replaces_non_ascii);
    RUN_TEST(test_draw_text_advances_and_clips);
    RUN_TEST(test_invert_rect);
    return UNITY_END();
}

// Back Button (Top-Left)
    lv_obj_t* back_btn = lv_btn_create(passhold for screen name);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x333333), 0); // Darker gray button
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT); // Left arrow icon
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        (passhold for screen name)();
    }, LV_EVENT_CLICKED, NULL);

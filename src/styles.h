#pragma once
#include <QString>

namespace Styles {

inline const QString lightQss = R"(
QWidget {
    font-family: "JetBrains Mono";
    font-size: 14px;
    color: #231A34;
    border: none;
}

QMainWindow,
QWidget#CentralFrame,
QWidget#CentralWidget {
    background: #F6F4FB;
}

#ExamDropArea {
    border-radius: 16px;
    border: 1px dashed rgba(148, 163, 184, 0.9);
    background: rgba(248, 250, 252, 0.6);
}

#ExamDropArea[dragActive="true"] {
    border-style: solid;
    border-color: #7C5CFA;
    background: rgba(129, 140, 248, 0.12);
}

/* --- COMMON HOSTS (light) --- */

QWidget#PlainHost,
QWidget#TransparentHost,
QWidget#BrandRow,
QWidget#ContentHost,
QWidget#DashboardExamHost,
QWidget#SeizureFormHost,
QWidget#SeizureHistoryHost,
QWidget#MedCardsHost,
QWidget#MedicationFormHost,
QWidget#DayMedicationHost,
QWidget#ExamFormHost,
QWidget#PastExamHost,
QWidget#PlannedExamHost,
QWidget#AvatarCardHost,
QWidget#ProfileFormHost {
    background: transparent;
}

/* --- SIDEBAR / NAV (light) --- */

QFrame#Sidebar {
    background: #EEEAFB;
    border: 1px solid #DDD5F3;
    border-radius: 28px;
}

QLabel#LogoTrackly {
    font-size: 24px;
    font-weight: 800;
    color: #20172E;
    background: transparent;
}

QLabel#LogoDesktop {
    font-size: 24px;
    font-weight: 300;
    color: #786C98;
    background: transparent;
    margin-top: 0px;
}

QLabel#PageTitle {
    font-size: 30px;
    font-weight: 800;
    color: #20172E;
    background: transparent;
}

QPushButton#SidebarNavButton {
    text-align: left;
    padding: 12px 14px;
    border-radius: 16px;
    background: transparent;
    color: #5D5379;
    font-weight: 650;
    width: 100%;
}

QPushButton#SidebarNavButton:hover {
    background: #E5DEF8;
    color: #2B2144;
}

QPushButton#SidebarNavButton:checked {
    background: #7C5CFA;
    color: white;
}

/* --- HELP BUTTON / TOAST (light) --- */

QToolButton#HelpButton {
    background: #E8E1FB;
    border: none;
    border-radius: 14px;
    min-width: 38px;
    max-width: 38px;
    min-height: 38px;
    max-height: 38px;
}

QToolButton#HelpButton:hover {
    background: #DDD2FB;
}

QWidget#ToastHost {
    background: transparent;
}

/* --- CARDS (light) --- */

QFrame#SectionCard,
QFrame#FeedCard,
QFrame#MarketplaceCard {
    background: #FCFBFF;
    border: 1px solid #E4DCF8;
    border-radius: 24px;
}

QPushButton#SummaryCard {
    background: #FCFBFF;
    border: 1px solid #E4DCF8;
    border-radius: 24px;
    text-align: left;
    min-height: 148px;
}

QPushButton#SummaryCard:hover {
    background: #F8F4FF;
    border: 1px solid #D0C0FF;
}

/* --- TYPOGRAPHY (light) --- */

QLabel#SummaryTitle {
    background: transparent;
    color: #766A95;
    font-size: 13px;
    font-weight: 650;
}

QLabel#SummaryValue {
    background: transparent;
    color: #211730;
    font-size: 22px;
    font-weight: 760;
}

QLabel#SummaryHint {
    background: transparent;
    color: #8C82A8;
    font-size: 13px;
}

QLabel#SectionTitle {
    background: transparent;
    font-size: 20px;
    font-weight: 760;
    color: #231A34;
}

QLabel#CardTitle {
    background: transparent;
    font-size: 17px;
    font-weight: 760;
    color: #231A34;
}

QLabel#MutedLabel,
QLabel#MiniCardLabel {
    background: transparent;
    color: #7C7198;
}

QLineEdit#ProfileNameEdit {
    background: transparent;
    border: none;
    padding: 0;
    font-size: 22px;
    font-weight: 700;
    color: #1A1A2E;
}

QLineEdit#ProfileEmailEdit {
    background: transparent;
    border: none;
    padding: 0;
    font-size: 13px;
    color: #7B7B9A;
}

QLabel#BadgeLabel {
    background: #F1ECFF;
    color: #5C45D3;
    border: 1px solid #E2D8FF;
    border-radius: 14px;
    padding: 5px 10px;
    font-weight: 700;
}

/* --- BUTTONS (light) --- */

QPushButton#PrimaryButton {
    background: #7C5CFA;
    color: white;
    border: none;
    border-radius: 16px;
    padding: 12px 16px;
    font-weight: 760;
}

QPushButton#PrimaryButton:hover {
    background: #6C4CF1;
}

QPushButton#SecondaryButton {
    background: #EEE8FF;
    color: #5439C9;
    border: none;
    border-radius: 16px;
    padding: 12px 16px;
    font-weight: 700;
}

QPushButton#SecondaryButton:hover {
    background: #E5DCFF;
}

QPushButton#DangerButton {
    background: #FFE9EC;
    color: #B8455C;
    border: none;
    border-radius: 16px;
    padding: 12px 16px;
    font-weight: 700;
}

QPushButton#DangerButton:hover {
    background: #FFDDE3;
}

QPushButton#GhostButton {
    background: transparent;
    border: none;
    color: #5D5379;
    font-weight: 600;
    padding: 4px 2px;
}

QPushButton#GhostButton:hover {
    color: #2B2144;
    text-decoration: underline;
}

/* --- FORM CONTROLS (light) --- */

QLineEdit,
QTextEdit,
QComboBox,
QDateTimeEdit,
QSpinBox {
    background: #FFFFFF;
    border: 1px solid #DED6F2;
    border-radius: 16px;
    padding: 10px 12px;
    color: #221A31;
    selection-background-color: #7C5CFA;
    selection-color: white;
}

QLineEdit:disabled,
QTextEdit:disabled,
QLineEdit[readOnly="true"],
QTextEdit[readOnly="true"] {
    color: #A5A0B8;
    background: #1A1525;
}

QLineEdit:focus,
QTextEdit:focus,
QComboBox:focus,
QDateTimeEdit:focus,
QSpinBox:focus {
    border: 1px solid #8C6BFF;
}

/* QComboBox arrow: минимализм, без квадрата */

QComboBox::drop-down {
    border: none;
    width: 26px;
    margin: 0px;
}

QComboBox::down-arrow {
    image: url(:/icons/chevron-down.svg);
    width: 14px;
    height: 14px;
}

QComboBox QAbstractItemView {
    background: #FFFEFF;
    color: #241A34;
    selection-background-color: #F0EAFF;
    selection-color: #231A34;
    border: 1px solid #E2D8F0;
    border-radius: 10px;
    padding: 4px;
    outline: none;
}

QDateEdit::drop-down,
QDateTimeEdit::drop-down {
    border: none;
    width: 26px;
    background: transparent;
}

QDateEdit::down-arrow,
QDateTimeEdit::down-arrow {
    image: url(:/icons/chevron-down.svg);
    width: 14px;
    height: 14px;
}

/* Спинбокс стрелки – прячем рамки */

QSpinBox::up-button,
QSpinBox::down-button {
    border: none;
    width: 20px;
}

QSpinBox::up-arrow {
    image: url(:/icons/chevron-up-mini.svg);
}

QSpinBox::down-arrow {
    image: url(:/icons/chevron-down-mini.svg);
}

/* --- SCROLL AREAS / SCROLLBARS (light) --- */

QScrollArea {
    border: none;
    background: transparent;
}

QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 4px 2px 4px 2px;
}

QScrollBar::handle:vertical {
    background: rgba(148, 163, 184, 0.65);
    border-radius: 999px;
    min-height: 24px;
}

QScrollBar::handle:vertical:hover {
    background: rgba(129, 140, 248, 0.85);
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0px;
    border: none;
    background: transparent;
}

QScrollBar:horizontal {
    background: transparent;
    height: 8px;
    margin: 2px 4px 2px 4px;
}

QScrollBar::handle:horizontal {
    background: rgba(148, 163, 184, 0.65);
    border-radius: 999px;
    min-width: 24px;
}

QScrollBar::handle:horizontal:hover {
    background: rgba(129, 140, 248, 0.85);
}

QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal {
    width: 0px;
    border: none;
    background: transparent;
}

/* --- CALENDAR (light) --- */

QCalendarWidget QWidget {
    background: transparent;
    alternate-background-color: transparent;
}

QCalendarWidget QToolButton {
    color: #2A2040;
    font-weight: 760;
    background: transparent;
    border: none;
    margin: 6px;
    padding: 8px;
    border-radius: 12px;
}

QCalendarWidget QToolButton:hover {
    background: #EEE8FF;
}

QCalendarWidget QAbstractItemView {
    background: #FFFEFF;
    color: #241A34;
    selection-background-color: #7C5CFA;
    selection-color: white;
    outline: 0;
    border: 1px solid #E1D8F4;
    border-radius: 18px;
    gridline-color: transparent;
    padding: 10px;
}

QCalendarWidget QWidget#qt_calendar_navigationbar {
    background: #FFFEFF;
    border-bottom: 1px solid #E1D8F4;
}

QCalendarWidget QMenu {
    background: #FFFEFF;
    color: #241A34;
    border: 1px solid #E1D8F4;
    border-radius: 10px;
    padding: 4px;
}

/* --- AVATAR / HELP OVERLAY (light) --- */

QLabel#AvatarPreview {
    background: #F1ECFF;
    border: 1px solid #E1D8F4;
    border-radius: 66px;
    color: #6C5B99;
    font-weight: 700;
}

QWidget#HelpOverlay {
    background: rgba(28, 18, 48, 90);
}

QFrame#HelpOverlayCard {
    background: #FFFCFF;
    border: 1px solid #E4DCF8;
    border-radius: 28px;
}

QFrame#HelpOverlayCard QLabel#SectionTitle {
    font-size: 16px;
    font-weight: 700;
    color: #231A34;
    margin-bottom: 4px;
}

QFrame#HelpOverlayCard QLabel#MutedLabel {
    font-size: 12px;
    color: #786C98;
    margin-bottom: 16px;
}

QLabel#HelpStepText {
    font-size: 13px;
    color: #3D3555;
    line-height: 1.5;
}

QLabel#HelpOverlayWarning {
    background: #FEF2F2;
    border-left: 3px solid #DC2626;
    border-radius: 0 8px 8px 0;
    padding: 10px 14px;
    font-size: 13px;
    color: #991B1B;
    line-height: 1.5;
}

QLabel#HelpOverlayText {
    background: transparent;
    color: #4B4362;
    font-size: 15px;
    line-height: 1.5;
}

/* --- AUTH: OTP (light) --- */

QWidget#AuthOtpHost {
    background: transparent;
}

QLabel#AuthOtpLabel {
    background: transparent;
    color: #7C7198;
    font-size: 13px;
}

QLineEdit#AuthOtpEdit {
    min-width: 40px;
    max-width: 40px;
    min-height: 48px;
    max-height: 48px;
    border-radius: 16px;
    border: 1px solid #DED6F2;
    background: #FFFFFF;
    font-size: 22px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
}

QLineEdit#AuthOtpEdit[state="error"] {
    border: 1px solid #E56A7A;
    background: #FFF3F5;
}

QLineEdit#AuthOtpEdit[state="success"] {
    border: 1px solid #4FB98A;
    background: #EDF9F4;
}

/* --- AUTH: overlay(card, texts) (light) --- */

#AuthOverlay {
    background: rgba(255, 255, 255, 0.95);
}

#AuthOverlayCard {
    background: #ffffff;
    border-radius: 28px;
    border: 1px solid rgba(15, 23, 42, 0.06);
}

#AuthOverlayTitle {
    font-size: 22px;
    font-weight: 700;
}

#AuthOverlaySubtitle {
    font-size: 13px;
    color: #6b7280;
}

#AuthOverlayCard QLineEdit {
    border-radius: 14px;
    border: 1px solid rgba(148, 163, 184, 0.8);
    padding: 9px 12px;
    background: rgba(248, 250, 252, 0.95);
    font-size: 14px;
}

#AuthOverlayCard QLineEdit:focus {
    border-color: #4f46e5;
    background: #ffffff;
}

#AuthAgreeCheckbox {
    font-size: 12px;
    color: #4b5563;
}

#AuthStatusLabel {
    font-size: 12px;
    color: #6b21a8;
}

/* Чекбоксы согласий в light-теме */

QCheckBox#AuthConsentCheckbox::indicator {
    width: 18px;
    height: 18px;
    border-radius: 6px;
    border: 1px solid #C4B8EA;
    background: #F8F5FF;
}

QCheckBox#AuthConsentCheckbox::indicator:checked {
    border-color: #7C5CFA;
    background: #7C5CFA;
    image: url(:/icons/check.svg);
}

QCheckBox#AuthConsentCheckbox::indicator:hover {
    border-color: #8C6BFF;
}

/* --- FEED CARDS (light) --- */

QFrame#FeedCardTaken {
    background: #E6F6EA;
    border-radius: 16px;
}

QFrame#FeedCardMissed {
    background: #FDE7E9;
    border-radius: 16px;
}

QLabel#LowStockWarning {
    color: #DC2626;
    font-weight: bold;
    font-size: 12px;
    background: transparent;
}

QPushButton#AiFabButton {
    background: #74AA9C;
    border-radius: 28px;
    border: none;
}
QPushButton#AiFabButton:hover {
    background: #5E9A8C;
}
QPushButton#AiTopbarButton {
    background: rgba(116, 170, 156, 0.12);
    border-radius: 8px;
    border: none;
    color: #5E9A8C;
    font-weight: bold;
    font-size: 12px;
    padding: 4px 12px;
}
QPushButton#AiTopbarButton:hover {
    background: rgba(116, 170, 156, 0.25);
}
QLabel#AiResultCard {
    background: #FFFFFF;
    border-radius: 16px;
    border: 1px solid rgba(116, 170, 156, 0.3);
    padding: 0;
}
QPushButton#AiCloseBtn {
    background: transparent;
    color: #94A3B8;
    font-size: 16px;
    font-weight: bold;
    border: none;
}
QPushButton#AiCloseBtn:hover {
    color: #DC2626;
}

/* --- INTAKE CELLS (приёмы за день, light) --- */

QFrame#IntakeCell {
    background: #FFFFFF;
    border-radius: 14px;
    border: 1px solid rgba(15, 23, 42, 0.06);
}

QFrame#IntakeCell:hover {
    background: rgba(148, 163, 184, 0.06);
}

/* статус через property status */

QFrame#IntakeCell[status="taken"] {
    border-color: rgba(74, 222, 128, 0.6);
}

QFrame#IntakeCell[status="missed"] {
    border-color: rgba(248, 113, 113, 0.7);
}

QFrame#IntakeCell[status="planned"] {
    border-color: rgba(129, 140, 248, 0.7);
}

/* подписи внутри клетки */

QFrame#IntakeCell QLabel[role="time"] {
    font-size: 13px;
    font-weight: 600;
    color: #0F172A;
}

QFrame#IntakeCell QLabel[role="badge"] {
    padding: 2px 8px;
    border-radius: 999px;
    font-size: 11px;
    font-weight: 500;
    background-color: rgba(148, 163, 184, 0.18);
    color: #0F172A;
}

/* --- QMENU (контекстные меню, light) --- */

QMenu {
    background-color: #FFFFFF;
    border-radius: 12px;
    border: 1px solid rgba(15, 23, 42, 0.08);
    padding: 6px 0;
    margin: 4px;
}

QMenu::item {
    padding: 6px 18px;
    border-radius: 8px;
    color: #0F172A;
    font-size: 13px;
}

QMenu::item:selected {
    background-color: rgba(129, 140, 248, 0.18);
    color: #111827;
}

QMenu::item:disabled {
    color: #9CA3AF;
    background-color: transparent;
}

QMenu::separator {
    height: 1px;
    margin: 4px 10px;
    background-color: rgba(148, 163, 184, 0.65);
}

/* индикаторы чекбоксов/радио в меню */

QMenu::indicator {
    width: 14px;
    height: 14px;
}
)";


inline const QString darkQss = R"(
QWidget {
    font-family: "JetBrains Mono";
    font-size: 14px;
    color: #F3EEFF;
    border: none;
}

QWidget#ToastHost {
    background: transparent;
}

QMainWindow,
QWidget#CentralFrame,
QWidget#CentralWidget {
    background: #17141E;
}

#ExamDropArea {
    border-radius: 16px;
    border: 1px dashed rgba(148, 163, 184, 0.9);
    background: rgba(15, 23, 42, 0.7);
}

#ExamDropArea[dragActive="true"] {
    border-style: solid;
    border-color: #C4B5FD;
    background: rgba(129, 140, 248, 0.18);
}

/* --- COMMON HOSTS (dark) --- */

QWidget#PlainHost,
QWidget#TransparentHost,
QWidget#BrandRow,
QWidget#ContentHost,
QWidget#DashboardExamHost,
QWidget#SeizureFormHost,
QWidget#SeizureHistoryHost,
QWidget#MedCardsHost,
QWidget#MedicationFormHost,
QWidget#DayMedicationHost,
QWidget#ExamFormHost,
QWidget#PastExamHost,
QWidget#PlannedExamHost,
QWidget#AvatarCardHost,
QWidget#ProfileFormHost {
    background: transparent;
}

/* --- SIDEBAR / NAV (dark) --- */

QFrame#Sidebar {
    background: #1D1827;
    border: 1px solid #2D263A;
    border-radius: 28px;
}

QLabel#LogoTrackly {
    font-size: 24px;
    font-weight: 800;
    color: #F8F3FF;
    background: transparent;
}

QLabel#LogoDesktop {
    font-size: 24px;
    font-weight: 300;
    color: #A89CC7;
    background: transparent;
    margin-top: 0px;
}

QLabel#PageTitle {
    font-size: 30px;
    font-weight: 800;
    color: #F8F3FF;
    background: transparent;
}

QPushButton#SidebarNavButton {
    text-align: left;
    padding: 12px 14px;
    border-radius: 16px;
    background: transparent;
    color: #B6ACD2;
    font-weight: 650;
    width: 100%;
}

QPushButton#SidebarNavButton:hover {
    background: #292235;
    color: #F7F2FF;
}

QPushButton#SidebarNavButton:checked {
    background: #8E74FF;
    color: white;
}

/* --- HELP BUTTON (dark) --- */

QToolButton#HelpButton {
    background: #2A2238;
    border: none;
    border-radius: 14px;
    min-width: 38px;
    max-width: 38px;
    min-height: 38px;
    max-height: 38px;
}

QToolButton#HelpButton:hover {
    background: #362C48;
}

/* --- CARDS (dark) --- */

QFrame#SectionCard,
QFrame#FeedCard,
QFrame#MarketplaceCard {
    background: #211B2B;
    border: 1px solid #342C43;
    border-radius: 24px;
}

QPushButton#SummaryCard {
    background: #211B2B;
    border: 1px solid #342C43;
    border-radius: 24px;
    text-align: left;
    min-height: 148px;
}

QPushButton#SummaryCard:hover {
    background: #281F36;
    border: 1px solid #5947A4;
}

/* --- TYPOGRAPHY (dark) --- */

QLabel#SummaryTitle {
    background: transparent;
    color: #B3A7D0;
    font-size: 13px;
    font-weight: 650;
}

QLabel#SummaryValue {
    background: transparent;
    color: #F7F2FF;
    font-size: 22px;
    font-weight: 760;
}

QLabel#SummaryHint {
    background: transparent;
    color: #9588B3;
    font-size: 13px;
}

QLabel#SectionTitle {
    background: transparent;
    font-size: 20px;
    font-weight: 760;
    color: #F7F2FF;
}

QLabel#CardTitle {
    background: transparent;
    font-size: 17px;
    font-weight: 760;
    color: #F6F1FF;
}

QLabel#MutedLabel,
QLabel#MiniCardLabel {
    background: transparent;
    color: #A094BE;
}

QLineEdit#ProfileNameEdit {
    background: transparent;
    border: none;
    padding: 0;
    font-size: 22px;
    font-weight: 700;
    color: #F6F1FF;
}

QLineEdit#ProfileEmailEdit {
    background: transparent;
    border: none;
    padding: 0;
    font-size: 13px;
    color: #A094BE;
}

QLabel#BadgeLabel {
    background: #2C2444;
    color: #C9BCFF;
    border: 1px solid #43385E;
    border-radius: 14px;
    padding: 5px 10px;
    font-weight: 700;
}

/* --- BUTTONS (dark) --- */

QPushButton#PrimaryButton {
    background: #8E74FF;
    color: white;
    border: none;
    border-radius: 16px;
    padding: 12px 16px;
    font-weight: 760;
}

QPushButton#PrimaryButton:hover {
    background: #9A84FF;
}

QPushButton#SecondaryButton {
    background: #2E2540;
    color: #D6CBFF;
    border: none;
    border-radius: 16px;
    padding: 12px 16px;
    font-weight: 700;
}

QPushButton#SecondaryButton:hover {
    background: #382D4C;
}

QPushButton#DangerButton {
    background: #402330;
    color: #FF9FB5;
    border: none;
    border-radius: 16px;
    padding: 12px 16px;
    font-weight: 700;
}

QPushButton#DangerButton:hover {
    background: #4A2837;
}

QPushButton#GhostButton {
    background: transparent;
    border: none;
    color: #B6ACD2;
    font-weight: 600;
    padding: 4px 2px;
}

QPushButton#GhostButton:hover {
    color: #F7F2FF;
    text-decoration: underline;
}

/* --- FORM CONTROLS (dark) --- */

QLineEdit,
QTextEdit,
QComboBox,
QDateTimeEdit,
QSpinBox {
    background: #1F1929;
    border: 1px solid #362C48;
    border-radius: 16px;
    padding: 10px 12px;
    color: #F5F0FF;
    selection-background-color: #8E74FF;
    selection-color: white;
}

QLineEdit:disabled,
QTextEdit:disabled,
QLineEdit[readOnly="true"],
QTextEdit[readOnly="true"] {
    color: #A5A0B8;
    background: #1A1525;
}

QLineEdit:focus,
QTextEdit:focus,
QComboBox:focus,
QDateTimeEdit:focus,
QSpinBox:focus {
    border: 1px solid #9D86FF;
}

/* QComboBox arrow (dark) */

QComboBox::drop-down {
    border: none;
    width: 26px;
    margin: 0px;
}

QComboBox::down-arrow {
    image: url(:/icons/chevron-down.svg);
    width: 14px;
    height: 14px;
}

QComboBox QAbstractItemView {
    background: #1F1929;
    color: #F4EEFF;
    selection-background-color: #2F2740;
    selection-color: #F4EEFF;
    border: 1px solid #3A3050;
    border-radius: 10px;
    padding: 4px;
    outline: none;
}

QDateEdit::drop-down,
QDateTimeEdit::drop-down {
    border: none;
    width: 26px;
    background: transparent;
}

QDateEdit::down-arrow,
QDateTimeEdit::down-arrow {
    image: url(:/icons/chevron-down.svg);
    width: 14px;
    height: 14px;
}

/* SpinBox arrows (dark) */

QSpinBox::up-button,
QSpinBox::down-button {
    border: none;
    width: 20px;
}

QSpinBox::up-arrow {
    image: url(:/icons/chevron-up-mini.svg);
}

QSpinBox::down-arrow {
    image: url(:/icons/chevron-down-mini.svg);
}

/* --- SCROLLBARS (dark, минимализм) --- */

QScrollArea {
    border: none;
    background: transparent;
}

QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 4px 2px 4px 2px;
}

QScrollBar::handle:vertical {
    background: rgba(148, 163, 184, 0.7);
    border-radius: 999px;
    min-height: 24px;
}

QScrollBar::handle:vertical:hover {
    background: rgba(129, 140, 248, 0.95);
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0px;
    border: none;
    background: transparent;
}

QScrollBar:horizontal {
    background: transparent;
    height: 8px;
    margin: 2px 4px 2px 4px;
}

QScrollBar::handle:horizontal {
    background: rgba(148, 163, 184, 0.7);
    border-radius: 999px;
    min-width: 24px;
}

QScrollBar::handle:horizontal:hover {
    background: rgba(129, 140, 248, 0.95);
}

QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal {
    width: 0px;
    border: none;
    background: transparent;
}

/* --- CALENDAR (dark) --- */

QCalendarWidget QWidget {
    background: transparent;
    alternate-background-color: transparent;
}

QCalendarWidget QToolButton {
    color: #EEE8FF;
    font-weight: 760;
    background: transparent;
    border: none;
    margin: 6px;
    padding: 8px;
    border-radius: 12px;
}

QCalendarWidget QToolButton:hover {
    background: #2F2740;
}

QCalendarWidget QAbstractItemView {
    background: #1F1929;
    color: #F4EEFF;
    selection-background-color: #8E74FF;
    selection-color: white;
    outline: 0;
    border: 1px solid #3A304C;
    border-radius: 18px;
    gridline-color: transparent;
    padding: 10px;
}

QCalendarWidget QWidget#qt_calendar_navigationbar {
    background: #1F1929;
    border-bottom: 1px solid #3A304C;
}

QCalendarWidget QMenu {
    background: #1F1929;
    color: #F4EEFF;
    border: 1px solid #3A304C;
    border-radius: 10px;
    padding: 4px;
}

/* --- AVATAR / HELP (dark) --- */

QLabel#AvatarPreview {
    background: #2C2444;
    border: 1px solid #3B304D;
    border-radius: 66px;
    color: #D7CCFF;
    font-weight: 700;
}

QWidget#HelpOverlay {
    background: rgba(5, 4, 10, 120);
}

QFrame#HelpOverlayCard {
    background: #221B2E;
    border: 1px solid #3A304C;
    border-radius: 28px;
}

QFrame#HelpOverlayCard QLabel#SectionTitle {
    font-size: 16px;
    font-weight: 700;
    color: #F3EEFF;
    margin-bottom: 4px;
}

QFrame#HelpOverlayCard QLabel#MutedLabel {
    font-size: 12px;
    color: #94A3B8;
    margin-bottom: 16px;
}

QLabel#HelpStepText {
    font-size: 13px;
    color: #D0CCE0;
    line-height: 1.5;
}

QLabel#HelpOverlayWarning {
    background: #3B1515;
    border-left: 3px solid #DC2626;
    border-radius: 0 8px 8px 0;
    padding: 10px 14px;
    font-size: 13px;
    color: #FCA5A5;
    line-height: 1.5;
}

/* --- AUTH: overlay (dark) --- */

#AuthOverlay {
    background: rgba(15, 23, 42, 0.88);
}

#AuthOverlayCard {
    background: #020617;
    border-radius: 28px;
    border: 1px solid rgba(148, 163, 184, 0.35);
}

#AuthOverlayTitle {
    font-size: 22px;
    font-weight: 700;
    color: #e5e7eb;
}

#AuthOverlaySubtitle {
    font-size: 13px;
    color: #9ca3af;
}

#AuthOverlayCard QLineEdit {
    border-radius: 14px;
    border: 1px solid rgba(148, 163, 184, 0.6);
    padding: 9px 12px;
    background: rgba(15, 23, 42, 0.85);
    color: #e5e7eb;
    selection-background-color: #4f46e5;
}

#AuthOverlayCard QLineEdit:focus {
    border-color: #a855f7;
    background: rgba(15, 23, 42, 0.98);
}

#AuthAgreeCheckbox {
    font-size: 12px;
    color: #e5e7eb;
}

#AuthStatusLabel {
    font-size: 12px;
    color: #c4b5fd;
}

/* AUTH OTP (dark) */

QWidget#AuthOtpHost {
    background: transparent;
}

QLabel#AuthOtpLabel {
    background: transparent;
    color: #A094BE;
    font-size: 13px;
}

QLineEdit#AuthOtpEdit {
    min-width: 40px;
    max-width: 40px;
    min-height: 48px;
    max-height: 48px;
    border-radius: 16px;
    border: 1px solid #362C48;
    background: #1F1929;
    font-size: 22px;
    font-weight: 700;
    qproperty-alignment: AlignCenter;
    color: #F5F0FF;
}

QLineEdit#AuthOtpEdit[state="error"] {
    border: 1px solid #E56A7A;
    background: #3B2230;
}

QLineEdit#AuthOtpEdit[state="success"] {
    border: 1px solid #4FB98A;
    background: #1F2D28;
}

/* Чекбоксы согласий в dark-теме */

QCheckBox#AuthConsentCheckbox::indicator {
    width: 18px;
    height: 18px;
    border-radius: 6px;
    border: 1px solid #4B3F73;
    background: #1C1529;
}

QCheckBox#AuthConsentCheckbox::indicator:checked {
    border-color: #A78BFA;
    background: #A78BFA;
}

QCheckBox#AuthConsentCheckbox::indicator:hover {
    border-color: #C4B5FD;
}

/* FEED CARDS (dark) – у тебя пока те же цвета, можно будет затемнить */

QFrame#FeedCardTaken {
    background: #052E1F;   /* тёмно-зелёный, не бьёт по глазам */
    border-radius: 16px;
}

QFrame#FeedCardMissed {
    background: #3F1A23;   /* тёмно-красновато-бордовый */
    border-radius: 16px;
}

QLabel#LowStockWarning {
    color: #F87171;
    font-weight: bold;
    font-size: 12px;
    background: transparent;
}

QPushButton#AiFabButton {
    background: #74AA9C;
    border-radius: 28px;
    border: none;
}
QPushButton#AiFabButton:hover {
    background: #5E9A8C;
}
QPushButton#AiTopbarButton {
    background: rgba(116, 170, 156, 0.12);
    border-radius: 8px;
    border: none;
    color: #5E9A8C;
    font-weight: bold;
    font-size: 12px;
    padding: 4px 12px;
}
QPushButton#AiTopbarButton:hover {
    background: rgba(116, 170, 156, 0.25);
}
QLabel#AiResultCard {
    background: #1F1A2E;
    border-radius: 16px;
    border: 1px solid rgba(116, 170, 156, 0.25);
    padding: 0;
}
QPushButton#AiCloseBtn {
    background: transparent;
    color: #64748B;
    font-size: 16px;
    font-weight: bold;
    border: none;
}
QPushButton#AiCloseBtn:hover {
    color: #F87171;
}

/* --- INTAKE CELLS (приёмы за день, dark) --- */

QFrame#IntakeCell {
    background: #020617;
    border-radius: 14px;
    border: 1px solid rgba(148, 163, 184, 0.35);
}

QFrame#IntakeCell:hover {
    background: rgba(148, 163, 184, 0.20);
}

/* статус */

QFrame#IntakeCell[status="taken"] {
    border-color: rgba(74, 222, 128, 0.7);
}

QFrame#IntakeCell[status="missed"] {
    border-color: rgba(248, 113, 113, 0.9);
}

QFrame#IntakeCell[status="planned"] {
    border-color: rgba(129, 140, 248, 0.9);
}

QFrame#IntakeCell QLabel[role="time"] {
    font-size: 13px;
    font-weight: 600;
    color: #E5E7EB;
}

QFrame#IntakeCell QLabel[role="badge"] {
    padding: 2px 8px;
    border-radius: 999px;
    font-size: 11px;
    font-weight: 500;
    background-color: rgba(148, 163, 184, 0.3);
    color: #F9FAFB;
}

/* --- QMENU (контекстные меню, dark) --- */

QMenu {
    background-color: #020617;
    border-radius: 12px;
    border: 1px solid rgba(148, 163, 184, 0.35);
    padding: 6px 0;
    margin: 4px;
}

QMenu::item {
    padding: 6px 18px;
    border-radius: 8px;
    color: #E5E7EB;
    font-size: 13px;
}

QMenu::item:selected {
    background-color: rgba(129, 140, 248, 0.40);
    color: #F9FAFB;
}

QMenu::item:disabled {
    color: #6B7280;
    background-color: transparent;
}

QMenu::separator {
    height: 1px;
    margin: 4px 10px;
    background-color: rgba(55, 65, 81, 0.9);
}

QMenu::indicator {
    width: 14px;
    height: 14px;
}
)";

inline QString buildStylesheet(bool dark)
{
    return dark ? darkQss : lightQss;
}

} // namespace Styles

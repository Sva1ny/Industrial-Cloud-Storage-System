#pragma once
#include <QString>

namespace ThemeManager {

inline QString globalStyleSheet()
{
    return R"(
        * {
            font-family: "PingFang SC",
                         "Helvetica Neue", "Helvetica", "Arial", sans-serif;
        }

        QMainWindow, QWidget#authPage, QWidget#homePage {
            background-color: #F5F5F7;
        }

        /* ── QLineEdit ── */
        QLineEdit {
            padding: 0 14px;
            border: 1px solid #E5E5EA;
            border-radius: 10px;
            background: #FFFFFF;
            font-size: 14px;
            color: #1D1D1F;
            selection-background-color: #007AFF;
            selection-color: #FFFFFF;
            min-height: 20px;
            max-height: 44px;
        }
        QLineEdit:hover { border-color: #C7C7CC; }
        QLineEdit:focus {
            border: 2px solid #007AFF;
            padding: 0 13px;
        }
        QLineEdit:disabled {
            background: #F5F5F7;
            color: #86868B;
            border-color: #E5E5EA;
        }
        QLineEdit::placeholder { color: #B0B0B5; }

        /* ── QPushButton: Primary (gradient) ── */
        QPushButton#btnPrimary {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #007AFF, stop:1 #0062CC);
            color: #FFFFFF;
            border: none;
            border-radius: 10px;
            padding: 0 24px;
            font-size: 15px;
            font-weight: 600;
            min-height: 44px;
        }
        QPushButton#btnPrimary:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                                                    stop:0 #0062CC, stop:1 #004999); }
        QPushButton#btnPrimary:pressed { background: #004999; }
        QPushButton#btnPrimary:disabled {
            background: #99C9FF;
            color: rgba(255,255,255,0.7);
        }

        /* ── QPushButton: Secondary ── */
        QPushButton#btnSecondary {
            background-color: transparent;
            color: #007AFF;
            border: 1px solid #D2D2D7;
            border-radius: 8px;
            padding: 0 16px;
            font-size: 14px;
            font-weight: 500;
            min-height: 36px;
        }
        QPushButton#btnSecondary:hover {
            background-color: #F5F5F7;
            border-color: #C7C7CC;
        }
        QPushButton#btnSecondary:pressed { background-color: #E8E8ED; }
        QPushButton#btnSecondary:disabled {
            color: #B0B0B5;
            border-color: #E5E5EA;
        }

        /* ── QPushButton: Table action ── */
        QPushButton#btnTableAction {
            background: none;
            border: 1px solid transparent;
            border-radius: 4px;
            padding: 4px 10px;
            font-size: 12px;
            font-weight: 500;
            color: #007AFF;
            min-height: 16px;
        }
        QPushButton#btnTableAction:hover {
            background-color: #E8F0FE;
            border-color: #D2D2D7;
        }
        QPushButton#btnTableAction:pressed { background-color: #D0E2FF; }
        QPushButton#btnTableAction[state="destructive"] { color: #FF3B30; }
        QPushButton#btnTableAction[state="destructive"]:hover { background-color: #FFF0EF; }

        /* ── QPushButton: Link ── */
        QPushButton#btnLink {
            background: none;
            border: none;
            color: #007AFF;
            font-size: 13px;
            font-weight: 500;
            padding: 4px 0;
        }
        QPushButton#btnLink:hover { color: #0062CC; }
        QPushButton#btnLink:pressed { color: #004999; }

        /* ── QTableWidget ── */
        QTableWidget {
            background-color: #FFFFFF;
            border: none;
            gridline-color: transparent;
            selection-background-color: #E8F0FE;
            selection-color: #1D1D1F;
            font-size: 13px;
            color: #1D1D1F;
            outline: none;
        }
        QTableWidget::item {
            padding: 0 14px;
            min-height: 48px;
        }
        QTableWidget::item:selected {
            background-color: #E8F0FE;
            color: #1D1D1F;
        }
        QTableWidget::item:hover { background-color: #F5F7FA; }

        QHeaderView::section {
            background-color: #FAFAFA;
            color: #86868B;
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 0.5px;
            text-transform: uppercase;
            padding: 0 14px;
            border: none;
            border-bottom: 1px solid #E5E5EA;
        }
        QHeaderView::section:last { border-right: none; }

        /* ── QProgressBar ── */
        QProgressBar {
            border: none;
            border-radius: 3px;
            background: #F0F0F3;
            height: 6px;
            text-align: center;
            font-size: 11px;
            color: transparent;
        }
        QProgressBar::chunk {
            background-color: #007AFF;
            border-radius: 3px;
        }

        /* ── QLabel ── */
        QLabel { color: #1D1D1F; background: transparent; }
        QLabel#lblTitle {
            font-size: 26px;
            font-weight: 700;
            color: #1D1D1F;
        }
        QLabel#lblSubtitle {
            font-size: 13px;
            font-weight: 400;
            color: #86868B;
        }
        QLabel#lblUsername {
            font-size: 16px;
            font-weight: 600;
            color: #1D1D1F;
        }
        QLabel#lblMeta {
            font-size: 12px;
            font-weight: 400;
            color: #86868B;
        }
        QLabel#lblStatus {
            font-size: 13px;
            font-weight: 500;
            padding: 2px 0;
        }
        QLabel#lblStatus[state="success"] { color: #34C759; }
        QLabel#lblStatus[state="error"]   { color: #FF3B30; }
        QLabel#lblStatus[state="info"]    { color: #007AFF; }

        /* ── QFrame: Card ── */
        QFrame#card {
            background-color: #FFFFFF;
            border: 1px solid #E5E5EA;
            border-radius: 16px;
        }
        QFrame#topBar {
            background-color: #FFFFFF;
            border: none;
            border-bottom: 1px solid #D2D2D7;
        }

        /* ── ScrollBar ── */
        QScrollBar:vertical {
            border: none;
            background: transparent;
            width: 8px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #C7C7CC;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: #A8A8AD; }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal {
            border: none;
            background: transparent;
            height: 8px;
            margin: 0;
        }
        QScrollBar::handle:horizontal {
            background: #C7C7CC;
            border-radius: 4px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover { background: #A8A8AD; }
        QScrollBar::add-line:horizontal,
        QScrollBar::sub-line:horizontal { width: 0; }

        /* ── QCheckBox ── */
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 1.5px solid #C0C0C0;
            border-radius: 4px;
            background: #FFFFFF;
        }
        QCheckBox::indicator:checked {
            background: #007AFF;
            border-color: #007AFF;
        }

        /* ── QMessageBox ── */
        QMessageBox { background-color: #FFFFFF; }
        QMessageBox QLabel { color: #1D1D1F; font-size: 13px; }
        QMessageBox QPushButton {
            min-width: 72px;
            padding: 6px 20px;
            border: 1px solid #D2D2D7;
            border-radius: 6px;
            background: #FFFFFF;
            font-size: 13px;
        }
        QMessageBox QPushButton:hover { background: #F5F5F7; }
    )";
}

} // namespace ThemeManager

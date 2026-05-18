
#include "TranslationDialog.h"

// 初始化静态变量
double TranslationDialog::s_lastCuttingDirX = 0.0;
double TranslationDialog::s_lastCuttingDirY = 0.0;
double TranslationDialog::s_lastCuttingDirZ = 0.0;

TranslationDialog::TranslationDialog(QWidget* parent, int i, double x, double y, double z, int* m_depthoffset, double offsetditance, int offsetnumber) : QDialog(parent)
{
    
    switch (i)
    {
    case 1:
        move();
        break;
    case 2:
        rotate();
        break;
    case 3:
        scale();
        break;
	case 4:
		extend();
        break;
    case 5:
		EdgeDiscrete();
		break;
    case 6:
		InputCuttingVelocity(x,y,z);
        break;
    case 7:
        InputCuttingDepth(x,y,z, m_depthoffset, offsetditance, offsetnumber);
        break;
    default:
        break;
    }
   
}

void TranslationDialog::move()
{
    // 设置窗口标题
    setWindowTitle("输入平移距离");

    // 创建表单布局
    QFormLayout* formLayout = new QFormLayout(this);

    // 添加 X 方向输入框
    xLineEdit = new QLineEdit(this);
    xLineEdit->setPlaceholderText("X 方向 (mm)");
    formLayout->addRow("X 方向:", xLineEdit);

    // 添加 Y 方向输入框
    yLineEdit = new QLineEdit(this);
    yLineEdit->setPlaceholderText("Y 方向 (mm)");
    formLayout->addRow("Y 方向:", yLineEdit);

    // 添加 Z 方向输入框
    zLineEdit = new QLineEdit(this);
    zLineEdit->setPlaceholderText("Z 方向 (mm)");
    formLayout->addRow("Z 方向:", zLineEdit);

    // 添加绝对位置移动复选框
    absolutePositionCheckBox = new QCheckBox("绝对位置移动", this);
    absolutePositionCheckBox->setToolTip("勾选：移动到指定绝对位置\n不勾选：相对当前位置移动");
    formLayout->addRow("", absolutePositionCheckBox);

    // 添加按钮框（确定和取消）
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    formLayout->addWidget(buttonBox);

    // 连接按钮信号
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TranslationDialog::rotate()
{
    // 设置窗口标题
    setWindowTitle("输入旋转角度和旋转轴");

    // 创建表单布局
    QFormLayout* formLayout = new QFormLayout(this);

    //添加旋转角度输入框
    angleLineEdit = new QLineEdit(this);
    angleLineEdit->setPlaceholderText("旋转角度 (度)");
    formLayout->addRow("旋转角度:", angleLineEdit);

    // 添加 X 方向输入框
    xLineEdit = new QLineEdit(this);
    xLineEdit->setPlaceholderText("X 方向 (mm)");
    formLayout->addRow("X 方向:", xLineEdit);

    // 添加 Y 方向输入框
    yLineEdit = new QLineEdit(this);
    yLineEdit->setPlaceholderText("Y 方向 (mm)");
    formLayout->addRow("Y 方向:", yLineEdit);

    // 添加 Z 方向输入框
    zLineEdit = new QLineEdit(this);
    zLineEdit->setPlaceholderText("Z 方向 (mm)");
    formLayout->addRow("Z 方向:", zLineEdit);

    // 添加按钮框（确定和取消）
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    formLayout->addWidget(buttonBox);

    // 连接按钮信号
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TranslationDialog::scale()
{
    // 设置窗口标题
    setWindowTitle("输入缩放比例");
    // 创建表单布局
    QFormLayout* formLayout = new QFormLayout(this);

    // 1添加缩放比例输入框
    scaleEdit = new QLineEdit(this);
    scaleEdit->setPlaceholderText("缩放比例");
    formLayout->addRow("缩放比例:", scaleEdit);
    // 2添加 X 方向输入框
    xLineEdit = new QLineEdit(this);
    xLineEdit->setPlaceholderText("缩放点X坐标 (mm)");
    formLayout->addRow("X :", xLineEdit);
    // 3添加 Y 方向输入框
    yLineEdit = new QLineEdit(this);
    yLineEdit->setPlaceholderText("缩放点Y坐标 (mm)");
    formLayout->addRow("Y :", yLineEdit);
    // 4添加 Z 方向输入框
    zLineEdit = new QLineEdit(this);
    zLineEdit->setPlaceholderText("缩放点Z坐标 (mm)");
    formLayout->addRow("Z :", zLineEdit);

    // 添加按钮框（确定和取消）
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    formLayout->addWidget(buttonBox);
    // 连接按钮信号
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TranslationDialog::extend()
{
    // 设置窗口标题
    setWindowTitle("输入面扩展量");
    // 创建表单布局
    QFormLayout* formLayout = new QFormLayout(this);

    // 2添加 X 方向输入框
    xLineEdit = new QLineEdit(this);
    xLineEdit->setPlaceholderText("u+:");
    formLayout->addRow("u+:", xLineEdit);
    // 3添加 Y 方向输入框
    yLineEdit = new QLineEdit(this);
    yLineEdit->setPlaceholderText("v+:");
    formLayout->addRow("v+:", yLineEdit);
    // 4添加 Z 方向输入框
    zLineEdit = new QLineEdit(this);
    zLineEdit->setPlaceholderText("u-:");
    formLayout->addRow("u-", zLineEdit);
    // 4添加 Z 方向输入框
    angleLineEdit = new QLineEdit(this);
    angleLineEdit->setPlaceholderText("v-:");
    formLayout->addRow("v-", angleLineEdit);

    // 添加按钮框（确定和取消）
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    formLayout->addWidget(buttonBox);
    // 连接按钮信号
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TranslationDialog::EdgeDiscrete()
{
    // 设置窗口标题
    setWindowTitle("输入想要离散点的数量");
    // 创建表单布局
    QFormLayout* formLayout = new QFormLayout(this);
    discreteCountEdit = new QLineEdit(this);
    discreteCountEdit->setPlaceholderText("离散点数量");
    formLayout->addRow("离散点数量:", discreteCountEdit);

	// 添加按钮框（确定和取消）
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	formLayout->addWidget(buttonBox);
	// 连接按钮信号
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TranslationDialog::InputCuttingVelocity(double x, double y, double z)
{
    // 设置窗口标题
    setWindowTitle("输入切削速度（mm/s）");
    QFormLayout* formLayout = new QFormLayout(this);
    // 添加 X 方向输入框
    xLineEdit = new QLineEdit(this);
    xLineEdit->setPlaceholderText("X 方向 ");
    xLineEdit->setText(QString::number(x));
    formLayout->addRow("X 方向:", xLineEdit);

    // 添加 Y 方向输入框
    yLineEdit = new QLineEdit(this);
    yLineEdit->setPlaceholderText("Y 方向 ");
    yLineEdit->setText(QString::number(y));
    formLayout->addRow("Y 方向:", yLineEdit);

    // 添加 Z 方向输入框
    zLineEdit = new QLineEdit(this);
    zLineEdit->setPlaceholderText("Z 方向 ");
    zLineEdit->setText(QString::number(z));
    formLayout->addRow("Z 方向:", zLineEdit);

    // 添加按钮框（确定和取消）
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    formLayout->addWidget(buttonBox);

    // 连接按钮信号
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TranslationDialog::InputCuttingDepth(double x, double y, double z, int* m_depthoffset, double offsetdistance, int offsetnumber)
{
    // 设置窗口标题
    setWindowTitle("输入齿升量(mm)");
    QFormLayout* formLayout = new QFormLayout(this);
    // 添加 X 方向输入框
    xLineEdit = new QLineEdit(this);
    xLineEdit->setPlaceholderText("X 方向 ");
    xLineEdit->setText(QString::number(x));
    formLayout->addRow("X 方向:", xLineEdit);
    x_positive = new QCheckBox(this);    
    x_negative = new QCheckBox(this);
    x_positive->setChecked(m_depthoffset[0]);
    x_negative->setChecked(m_depthoffset[1]);
    formLayout->addRow("+X 齿升:", x_positive);
    formLayout->addRow("-X 齿升:", x_negative);

    // 添加 Y 方向输入框
    yLineEdit = new QLineEdit(this);
    yLineEdit->setPlaceholderText("Y 方向 ");
    yLineEdit->setText(QString::number(y));
    formLayout->addRow("Y 方向:", yLineEdit);
    y_positive = new QCheckBox(this);
    y_negative = new QCheckBox(this);
    y_positive->setChecked(m_depthoffset[2]);
    y_negative->setChecked(m_depthoffset[3]);
    formLayout->addRow("+Y 齿升:", y_positive);
    formLayout->addRow("-Y 齿升:", y_negative);
    // 添加 Z 方向输入框
    zLineEdit = new QLineEdit(this);
    zLineEdit->setPlaceholderText("Z 方向 ");
    zLineEdit->setText(QString::number(z));
    formLayout->addRow("Z 方向:", zLineEdit);
    z_positive = new QCheckBox(this);
    z_negative = new QCheckBox(this);
    z_positive->setChecked(m_depthoffset[4]);
    z_negative->setChecked(m_depthoffset[5]);
    formLayout->addRow("+Z 齿升:", z_positive);
    formLayout->addRow("-Z 齿升:", z_negative);

    // 偏置刀具
    QHBoxLayout* offsetLayout = new QHBoxLayout();
    QLabel* offsetLabel = new QLabel("偏置方向:");
    //QLineEdit* offsetDirectionEdit = new QLineEdit();
    // 创建下拉框选择偏置方向
    offsetDirectioncomboBox = new QComboBox();

    // 添加项目 - 方法1：逐个添加
    offsetDirectioncomboBox->addItem("-X");
    offsetDirectioncomboBox->addItem("+X");
    offsetDirectioncomboBox->addItem("-Y");
    offsetDirectioncomboBox->addItem("+Y");
    offsetDirectioncomboBox->addItem("-Z");
    offsetDirectioncomboBox->addItem("+Z");
    offsetDirectioncomboBox->setCurrentIndex(m_depthoffset[6]);
    QLabel* offsetDistanceLabel = new QLabel("间距:");
    offsetDistanceEdit = new QLineEdit();
    offsetDistanceEdit->setText(QString::number(offsetdistance));
    QLabel* edgeNumLabel = new QLabel("偏置刀刃数量:");
    edgeNumEdit = new QLineEdit();
    edgeNumEdit->setText(QString::number(offsetnumber));

    offsetLayout->addWidget(offsetLabel);
    offsetLayout->addWidget(offsetDirectioncomboBox);
    offsetLayout->addWidget(offsetDistanceLabel);
    offsetLayout->addWidget(offsetDistanceEdit);
    offsetLayout->addWidget(edgeNumLabel);
    offsetLayout->addWidget(edgeNumEdit);

    formLayout->addRow(offsetLayout);

    // 添加按钮框（确定和取消）
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    formLayout->addWidget(buttonBox);

    // 连接按钮信号
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

}

double TranslationDialog::getAngle()const
{
    return angleLineEdit->text().toDouble();
}

double TranslationDialog::getX() const
{
	return xLineEdit->text().toDouble();
}
double TranslationDialog::getY() const
{
	return yLineEdit->text().toDouble();
}
double TranslationDialog::getZ() const
{
	return zLineEdit->text().toDouble();
}
double TranslationDialog::getScale() const
{
	return scaleEdit->text().toDouble();
}

double TranslationDialog::getDiscreteCount() const
{
    return xLineEdit ? xLineEdit->text().toDouble() : 0.0;
} // 获取离散点数量

bool TranslationDialog::isAbsolutePosition() const
{
    return absolutePositionCheckBox ? absolutePositionCheckBox->isChecked() : false;
}
void TranslationDialog::getDepthDirection(double* direction, int sizeofdirection, int* depthoffset) const
{
    if (sizeofdirection < 11) return;
    if (x_positive->isChecked())depthoffset[0] = 1; else depthoffset[0] = 0;
    if (x_negative->isChecked())depthoffset[1] = 1; else depthoffset[1] = 0;
    if (y_positive->isChecked())depthoffset[2] = 1; else depthoffset[2] = 0;
    if (y_negative->isChecked())depthoffset[3] = 1; else depthoffset[3] = 0;
    if (z_positive->isChecked())depthoffset[4] = 1; else depthoffset[4] = 0;
    if (z_negative->isChecked())depthoffset[5] = 1; else depthoffset[5] = 0;
    depthoffset[6] = offsetDirectioncomboBox->currentIndex();

    if (x_positive->isChecked() && !x_negative->isChecked()) // 齿高为+X方向
    {
       //direction[0] = 1;
    }
    if (!x_positive->isChecked() && x_negative->isChecked()) // 齿高为-X方向
    {
        direction[0] = -1;
    }
    if (y_positive->isChecked() && !y_negative->isChecked()) // 齿高为+Y方向
    {
        //direction[1] = 3;
    }
    if (!y_positive->isChecked() && y_negative->isChecked()) // 齿高为-Y方向
    {
        direction[1] = -1;
    }
    if (z_positive->isChecked() && !z_negative->isChecked()) // 齿高为+Z方向
    {
        direction[2] = 1;
    }
    if (!z_positive->isChecked() && z_negative->isChecked()) // 齿高为-Z方向
    {
        direction[2] = -1;
    }

    if (x_positive->isChecked() && x_negative->isChecked()) // 齿厚为X方向
    {
        direction[3] = 1;
    }
    if (y_positive->isChecked() && y_negative->isChecked()) // 齿厚为Y方向
    {
        direction[4] = 1;
    }
    if (z_positive->isChecked() && z_negative->isChecked()) // 齿厚为Z方向
    {
        direction[5] = 1;
    }

    if (!x_positive->isChecked() && !x_negative->isChecked()) // 非齿升方向为X方向
    {
        direction[0] = 0;
    }
    if (!y_positive->isChecked() && !y_negative->isChecked()) // 非齿升方向为Y方向
    {
        direction[1] = 0;
    }
    if (!z_positive->isChecked() && !z_negative->isChecked()) // 非齿升方向为Z方向
    {
        direction[2] = 1;
    }

    switch (offsetDirectioncomboBox->currentIndex()) // 当前选中索引
    {
    case 0: //-X
        direction[6] = -1;
        direction[7] = 0;
        direction[8] = 0;
        break;
    case 1:
        direction[6] = 1;
        direction[7] = 0;
        direction[8] = 0;
        break;
    case 2: //-Y
        direction[6] = 0;
        direction[7] = -1;
        direction[8] = 0;
        break;
    case 3:
        direction[6] = 0;
        direction[7] = 1;
        direction[8] = 0;
        break;
    case 4: //-Z
        direction[6] = 0;
        direction[7] = 0;
        direction[8] = -1;
        break;
    case 5:
        direction[6] = 0;
        direction[7] = 0;
        direction[8] = 1;
        break;
    };  
    direction[9] = offsetDistanceEdit->text().toDouble();
    direction[10] = edgeNumEdit->text().toInt();
    //return direction;
} // 获取离散点数量

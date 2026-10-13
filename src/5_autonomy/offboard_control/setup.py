from setuptools import find_packages, setup

package_name = 'offboard_control'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='tim',
    maintainer_email='tim.gg.yi@gmail.com',
    description='PX4 offboard takeoff and landing controller for the ZD550 UAV.',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'offboard_takeoff_land = offboard_control.offboard_takeoff_land:main',
        ],
    },
)

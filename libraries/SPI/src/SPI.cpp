/*
 * Copyright (c) 2016 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#include "Arduino.h"
#include "stm32l4_wiring_private.h"
#include "SPI.h"

/* The code below deserves some explanation. The SPIClass has really 2 modes of operation.
 * One is the beginTransaction()/endTransaction() which locally braces every atomic transaction,
 * and one is the old model, where you'd configure the SPI PORT directly. For STM32L4 we
 * really, really only want to deal with the transaction model, as this allows to clock gate
 * the peripheral. Hence the transfer*() function are wrapped via indirect functions calls.
 * If a transfer*() function is called outside a transaction then a virtual beginTransaction()
 * is inserted, and if API are called the reconfigure the SPI PORT, then a virtual endTransaction()
 * is inserted.
 */

SPIClass::SPIClass(struct _stm32l4_spi_t *spi, unsigned int instance, const struct _stm32l4_spi_pins_t *pins, unsigned int priority, unsigned int mode)
{
    _spi = spi;

    stm32l4_spi_create(spi, instance, pins, priority, mode);

    _selected = false;

    _clock = 4000000;
    _bitOrder = MSBFIRST;
    _dataMode = SPI_MODE0;

    _reference = 16000000;

    _interruptMask = 0;

    _exchangeRoutine = SPIClass::_exchangeSelect;
    _exchange8Routine = SPIClass::_exchange8Select;
    _exchange16Routine = SPIClass::_exchange16Select;

    _completionCallback = NULL;
}

void SPIClass::begin()
{
    stm32l4_spi_enable(_spi, SPIClass::_eventCallback, (void*)this, (SPI_EVENT_RECEIVE_DONE | SPI_EVENT_TRANSMIT_DONE | SPI_EVENT_TRANSFER_DONE));
}

void SPIClass::end()
{
    if (_selected) {
	stm32l4_spi_unselect(_spi);

	_exchangeRoutine = SPIClass::_exchangeSelect;
	_exchange8Routine = SPIClass::_exchange8Select;
	_exchange16Routine = SPIClass::_exchange16Select;

	_selected = false;
    }

    stm32l4_spi_disable(_spi);
}

void SPIClass::usingInterrupt(uint32_t pin)
{
    if (!(g_APinDescription[pin].attr & PIN_ATTR_EXTI)) {
	return;
    }

    _interruptMask |= (1ul << ((g_APinDescription[pin].pin & GPIO_PIN_INDEX_MASK) >> GPIO_PIN_INDEX_SHIFT));
}

void SPIClass::notUsingInterrupt(uint32_t pin)
{
    if (!(g_APinDescription[pin].attr & PIN_ATTR_EXTI)) {
	return;
    }

    _interruptMask &= ~(1ul << ((g_APinDescription[pin].pin & GPIO_PIN_INDEX_MASK) >> GPIO_PIN_INDEX_SHIFT));
}

void SPIClass::beginTransaction(SPISettings settings)
{
    uint32_t option, clock, divide;

    option = settings._dataMode | ((settings._bitOrder == MSBFIRST) ? SPI_OPTION_MSB_FIRST : SPI_OPTION_LSB_FIRST);

    clock = stm32l4_spi_clock(_spi) / 2;
    divide = 0;

    while ((clock > settings._clock) && (divide < 7)) {
	clock /= 2;
	divide++;
    }

    option |= (divide << SPI_OPTION_DIV_SHIFT);

    if (_selected) {
	stm32l4_spi_configure(_spi, option);
    } else {
	_selected = true;

	_exchangeRoutine = stm32l4_spi_exchange;
	_exchange8Routine = stm32l4_spi_exchange8;
	_exchange16Routine = stm32l4_spi_exchange16;
    
	stm32l4_spi_select(_spi, option);
    }

    if (_interruptMask) {
	stm32l4_exti_suspend(&stm32l4_exti, _interruptMask);
    }
}

void SPIClass::endTransaction(void)
{
    if (_interruptMask) {
	stm32l4_exti_resume(&stm32l4_exti, _interruptMask);
    }

    stm32l4_spi_unselect(_spi);

    _exchangeRoutine = SPIClass::_exchangeSelect;
    _exchange8Routine = SPIClass::_exchange8Select;
    _exchange16Routine = SPIClass::_exchange16Select;

    _selected = false;
}

void SPIClass::setBitOrder(BitOrder bitOrder)
{
    if (_selected) {
	stm32l4_spi_unselect(_spi);
    
	_exchangeRoutine = SPIClass::_exchangeSelect;
	_exchange8Routine = SPIClass::_exchange8Select;
	_exchange16Routine = SPIClass::_exchange16Select;
    
	_selected = false;
    }
}

void SPIClass::setDataMode(uint8_t dataMode)
{
    if (_selected) {
	stm32l4_spi_unselect(_spi);
    
	_exchangeRoutine = SPIClass::_exchangeSelect;
	_exchange8Routine = SPIClass::_exchange8Select;
	_exchange16Routine = SPIClass::_exchange16Select;
    
	_selected = false;
    }

    _dataMode = dataMode;
}

void SPIClass::setClockDivider(uint8_t divider)
{
    if (_selected) {
	stm32l4_spi_unselect(_spi);
    
	_exchangeRoutine = SPIClass::_exchangeSelect;
	_exchange8Routine = SPIClass::_exchange8Select;
	_exchange16Routine = SPIClass::_exchange16Select;
    
	_selected = false;
    }

    if (divider != 0) {
	_clock = _reference / divider;
    }
}

void SPIClass::setClockDividerReference(uint32_t clock)
{
    _reference = clock;
}

void SPIClass::attachInterrupt()
{
  // Should be enableInterrupt()
}

void SPIClass::detachInterrupt()
{
  // Should be disableInterrupt()
}

bool SPIClass::transfer(const void *txBuffer, void *rxBuffer, size_t count, void(*callback)(void))
{
    if (!stm32l4_spi_done(_spi)) {
	return false;
    }

    if (!_selected) {
	uint32_t option, clock, divide;

	option = _dataMode | ((_bitOrder == MSBFIRST) ? SPI_OPTION_MSB_FIRST : SPI_OPTION_LSB_FIRST);

	clock = stm32l4_spi_clock(_spi) / 2;
	divide = 0;

	while ((clock > _clock) && (divide < 7)) {
	    clock /= 2;
	    divide++;
	}

	option |= (divide << SPI_OPTION_DIV_SHIFT);

	stm32l4_spi_select(_spi, option);

	_selected = true;

	_exchangeRoutine = stm32l4_spi_exchange;
	_exchange8Routine = stm32l4_spi_exchange8;
	_exchange16Routine = stm32l4_spi_exchange16;
    
    }

    _completionCallback = callback;

    if (rxBuffer) {
	if (txBuffer) {
	    if (!stm32l4_spi_transfer(_spi, static_cast<const uint8_t*>(txBuffer), static_cast<uint8_t*>(rxBuffer), count, 0)) {
		_completionCallback = NULL;

		return false;
	    }
	} else {
	    if (!stm32l4_spi_receive(_spi, static_cast<uint8_t*>(rxBuffer), count, 0)) {
		_completionCallback = NULL;

		return false;
	    }
	}
    } else {
	if (!stm32l4_spi_transmit(_spi, static_cast<const uint8_t*>(txBuffer), count, 0)) {
	    _completionCallback = NULL;

	    return false;
	}
    }

    return true;
}

void SPIClass::flush(void)
{
    if (armv7m_core_priority() <= STM32L4_SPI_IRQ_PRIORITY) {
	while (!stm32l4_spi_done(_spi)) {
	    stm32l4_spi_poll(_spi);
	}
    } else {
	while (!stm32l4_spi_done(_spi)) {
	    armv7m_core_yield();
	}
    }
}

bool SPIClass::done(void)
{
    return stm32l4_spi_done(_spi);
}

bool SPIClass::isEnabled(void)
{
    return (_spi->state >= SPI_STATE_READY);
}
    
void SPIClass::_exchangeSelect(struct _stm32l4_spi_t *spi, const uint8_t *txData, uint8_t *rxData, size_t count) 
{
    SPIClass *spi_class = reinterpret_cast<class SPIClass*>(spi->context);
    uint32_t option, clock, divide;

    option = spi_class->_dataMode | ((spi_class->_bitOrder == MSBFIRST) ? SPI_OPTION_MSB_FIRST : SPI_OPTION_LSB_FIRST);

    clock = stm32l4_spi_clock(spi) / 2;
    divide = 0;

    while ((clock > spi_class->_clock) && (divide < 7)) {
	clock /= 2;
	divide++;
    }
  
    option |= (divide << SPI_OPTION_DIV_SHIFT);
  
    stm32l4_spi_select(spi, option);

    spi_class->_selected = true;

    spi_class->_exchangeRoutine = stm32l4_spi_exchange;
    spi_class->_exchange8Routine = stm32l4_spi_exchange8;
    spi_class->_exchange16Routine = stm32l4_spi_exchange16;

    return (*spi_class->_exchangeRoutine)(spi, txData, rxData, count);
}

uint8_t SPIClass::_exchange8Select(struct _stm32l4_spi_t *spi, uint8_t data)
{
    SPIClass *spi_class = reinterpret_cast<class SPIClass*>(spi->context);
    uint32_t option, clock, divide;

    option = spi_class->_dataMode | ((spi_class->_bitOrder == MSBFIRST) ? SPI_OPTION_MSB_FIRST : SPI_OPTION_LSB_FIRST);

    clock = stm32l4_spi_clock(spi) / 2;
    divide = 0;

    while ((clock > spi_class->_clock) && (divide < 7)) {
	clock /= 2;
	divide++;
    }
  
    option |= (divide << SPI_OPTION_DIV_SHIFT);
  
    stm32l4_spi_select(spi, option);

    spi_class->_selected = true;

    spi_class->_exchangeRoutine = stm32l4_spi_exchange;
    spi_class->_exchange8Routine = stm32l4_spi_exchange8;
    spi_class->_exchange16Routine = stm32l4_spi_exchange16;

    return (*spi_class->_exchange8Routine)(spi, data);
}

uint16_t SPIClass::_exchange16Select(struct _stm32l4_spi_t *spi, uint16_t data)
{
    SPIClass *spi_class = reinterpret_cast<class SPIClass*>(spi->context);
    uint32_t option, clock, divide;

    option = spi_class->_dataMode | ((spi_class->_bitOrder == MSBFIRST) ? SPI_OPTION_MSB_FIRST : SPI_OPTION_LSB_FIRST);

    clock = stm32l4_spi_clock(spi) / 2;
    divide = 0;

    while ((clock > spi_class->_clock) && (divide < 7)) {
	clock /= 2;
	divide++;
    }
  
    option |= (divide << SPI_OPTION_DIV_SHIFT);
  
    stm32l4_spi_select(spi, option);

    spi_class->_selected = true;

    spi_class->_exchangeRoutine = stm32l4_spi_exchange;
    spi_class->_exchange8Routine = stm32l4_spi_exchange8;
    spi_class->_exchange16Routine = stm32l4_spi_exchange16;
  
    return (*spi_class->_exchange16Routine)(spi, data);
}

void SPIClass::EventCallback(uint32_t events)
{
    void(*callback)(void);
  
    callback = _completionCallback;
    _completionCallback = NULL;

    if (callback) {
	(*callback)();
    }
}

void SPIClass::_eventCallback(void *context, uint32_t events)
{
    reinterpret_cast<class SPIClass*>(context)->EventCallback(events);
}

#if SPI_INTERFACES_COUNT > 0

extern const stm32l4_spi_pins_t g_SPIPins;
extern const unsigned int g_SPIInstance;
extern const unsigned int g_SPIMode;

static stm32l4_spi_t _SPI;

SPIClass SPI(&_SPI, g_SPIInstance, &g_SPIPins, STM32L4_SPI_IRQ_PRIORITY, g_SPIMode);

#endif

#if SPI_INTERFACES_COUNT > 1

extern const stm32l4_spi_pins_t g_SPI1Pins;
extern const unsigned int g_SPI1Instance;
extern const unsigned int g_SPI1Mode;

static stm32l4_spi_t _SPI1;

SPIClass SPI1(&_SPI1, g_SPI1Instance, &g_SPI1Pins, STM32L4_SPI_IRQ_PRIORITY, g_SPI1Mode);

#endif

#if SPI_INTERFACES_COUNT > 2

extern const stm32l4_spi_pins_t g_SPI2Pins;
extern const unsigned int g_SPI2Instance;
extern const unsigned int g_SPI2Mode;

static stm32l4_spi_t _SPI2;

SPIClass SPI2(&_SPI2, g_SPI2Instance, &g_SPI2Pins, STM32L4_SPI_IRQ_PRIORITY, g_SPI2Mode);

#endif

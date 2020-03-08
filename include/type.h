#pragma once

typedef	unsigned long long	u64;
typedef	unsigned int		u32;
typedef	unsigned short		u16;
typedef	unsigned char		u8;

typedef	signed long long	i64;
typedef	signed int		    i32;
typedef	signed short		i16;
typedef	signed char		    i8;

typedef unsigned int size_t;

typedef	void	(*int_handler)	();
typedef	void	(*task_f)	();
typedef	void	(*irq_handler)	(int irq);

typedef void*	system_call;
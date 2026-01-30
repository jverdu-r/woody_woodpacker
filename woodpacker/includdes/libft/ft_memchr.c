/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_memchr.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jverdu-r <jverdu-r@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/01/20 15:37:55 by jverdu-r          #+#    #+#             */
/*   Updated: 2026/01/30 15:55:19 by jverdu-r         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stddef.h>

void	*ft_memchr(const void *s, int c, size_t n)
{
	unsigned long			a;
	const unsigned char		*p;

	a = 0;
	p = (const char unsigned *)s;
	while (a < n)
	{
		if (p[a] == (unsigned char)c)
			return ((void *)&p[a]);
		a++;
	}
	return (0);
}
